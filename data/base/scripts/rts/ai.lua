-- The opposing commander, for either faction: it follows its faction's plan (rts/defs.lua,
-- D.FACTIONS[f].ai). Every second it keeps its workers busy (gold, timber, bone for the
-- Wildborn), raises the next building of its plan (food ahead of folk; aether at a spring; the
-- headquarters' next stages), researches upgrades, trains its plan's units in turn, expands to a
-- second headquarters by fresh gold when its own mines run low, defends its headquarters and
-- attacks in growing waves. It plays by the same rules and sending stones as the player.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")
local E = require("rts.economy")
local C = require("rts.companies")
local P = require("rts.picture")
local Ti = require("rts.tiers")

local A = {}
A.SERFS = 16        -- workers per headquarters
A.FIRST_WAVE = 2    -- companies in the first attack; each wave one more, up to A.MAX_WAVE
A.MAX_WAVE = 6
A.SQUAD_MEN = 8     -- men counted as one company when sizing a wave
A.DEFEND = 70       -- metres around the headquarters that the AI defends
A.HOME = 150        -- mines within this of the headquarters are home mines
A.LOW = 0.25        -- expand when home mines hold less than this share of what they started with

local function plan(side_id) return D.FACTIONS[S.side(side_id).faction].ai end

-- A free spot for def near (x, z), trying rings outwards in a fixed order.
function A.find_spot(def_id, x, z, from, to)
  for r = from or 22, to or 70, 4 do
    local steps = math.max(8, math.floor(r))
    for i = 0, steps - 1 do
      local a = i / steps * 2 * math.pi
      local px, pz = x + math.cos(a) * r, z + math.sin(a) * r
      if B.can_place(def_id, px, pz) then return px, pz end
    end
  end
  return nil
end

local function count(list, pred)
  local n = 0
  for _, v in ipairs(list) do
    if pred(v) then n = n + 1 end
  end
  return n
end

-- Workers: trained up to A.SERFS per headquarters; idle ones build open sites or gather (gold
-- most, timber as it runs short, bone for the Wildborn while remains lie about).
function A.economy(side_id)
  local side = S.side(side_id)
  local hold = S.hq(side_id)
  if not hold then return end
  local worker = S.role(side_id, "worker")
  local serfs = S.units(side_id, worker)
  local hd = S.data(hold)
  if #serfs + #hd.queue < A.SERFS * #S.hqs(side_id) and #hd.queue == 0 then B.order(hold, worker) end
  local sites = {}
  for _, e in ipairs(S.structures(side_id)) do
    if not S.data(e).built then sites[#sites + 1] = e end
  end
  local builders = count(serfs, function(e) local t = S.data(e).task return t and t.t == "build" end)
  -- a site without enough builders takes workers off gathering (not while they carry a load)
  for _, e in ipairs(serfs) do
    if #sites == 0 or builders >= 2 then break end
    local d = S.data(e)
    if d.task and d.task.t == "gather" and not d.carry then
      E.build(e, sites[1])
      builders = builders + 1
    end
  end
  local gold = count(serfs, function(e) local t = S.data(e).task return t and t.res == "gold" end)
  local bone = count(serfs, function(e) local t = S.data(e).task return t and t.res == "bone" end)
  -- saving for something short of timber or gold: an empty-handed gatherer changes over
  local want = A.shortfall(side_id)
  if want == "timber" or want == "gold" then
    local from = want == "timber" and "gold" or "timber"
    for _, e in ipairs(serfs) do
      local d = S.data(e)
      if d.task and d.task.t == "gather" and d.task.res == from and not d.carry then
        if E.gather(e, want) and want == "gold" then gold = gold + 1 end
        if want == "timber" then gold = gold - 1 end
        break
      end
    end
  -- plenty of timber: an empty-handed woodcutter goes to the mines instead
  elseif side.timber > side.gold + 300 then
    for _, e in ipairs(serfs) do
      local d = S.data(e)
      if d.task and d.task.t == "gather" and d.task.res == "timber" and not d.carry then
        if E.gather(e, "gold") then gold = gold + 1 end
        break
      end
    end
  end
  local bones = D.unit(worker).bone
  for _, e in ipairs(serfs) do
    local d = S.data(e)
    if not d.task then
      if #sites > 0 and builders < 2 then
        E.build(e, sites[1])
        builders = builders + 1
      elseif bones and bone * 6 < #serfs and E.gather(e, "bone") then
        bone = bone + 1
      elseif side.timber < side.gold * 0.6 and gold * 3 >= #serfs then
        if not E.gather(e, "timber") then E.gather(e, "gold") end
      elseif gold * 2 < #serfs or side.timber > side.gold + 200 then
        if E.gather(e, "gold") then gold = gold + 1 end
      else
        if not E.gather(e, "timber") then E.gather(e, "gold") end
      end
    end
  end
  return side
end

-- What the side lacks most for the thing it saves up for ("gold", "timber", ... or nil).
function A.shortfall(side_id)
  local side = S.side(side_id)
  if not side.saving then return nil end
  local cost
  if side.saving == "hq_up" then
    local hq = S.hq(side_id)
    local next = hq and D.structure(D.structure(S.data(hq).def).next or "")
    cost = next and next.cost
  else
    cost = D.structure(side.saving) and D.structure(side.saving).cost
  end
  if not cost then return nil end
  local worst, gap = nil, 0
  for _, r in ipairs(S.RESOURCES) do
    local short = (cost[r] or 0) - (side[r] or 0)
    if short > gap then worst, gap = r, short end
  end
  return worst
end

-- The nearest free aether spring to (x, z) within 400 m, or nil.
function A.free_spring(x, z)
  local best, bd = nil, 400
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      if d.kind == "spring" and not (d.well and S.alive(d.well)) then
        local sx, _, sz = S.pos(e)
        local dist = S.dist(x, z, sx, sz)
        if dist < bd then best, bd = e, dist end
      end
    end
  end
  return best
end

-- Where to put a building of a role.
function A.site_for(side_id, role, def_id)
  local side = S.side(side_id)
  local hold = S.hq(side_id)
  local hx, _, hz = S.pos(hold)
  if role == "aether" then
    local spring = A.free_spring(hx, hz)
    if not spring then return nil end
    local sx, _, sz = S.pos(spring)
    return A.find_spot(def_id, sx, sz, 0, D.SPRING_REACH - 1)
  elseif role == "tower" then
    local enemy = S.hq(side.enemy)
    if enemy then
      local ex, _, ez = S.pos(enemy)
      local dx, dz = ex - hx, ez - hz
      local len = math.sqrt(dx * dx + dz * dz)
      return A.find_spot(def_id, hx + dx / len * 90, hz + dz / len * 90, 0, 40)
    end
  elseif role == "timber_drop" then
    local tree = E.nearest_source("timber", hx, hz, 400)
    if tree then
      local tx, _, tz = S.pos(tree)
      return A.find_spot(def_id, tx, tz, 12, 40)
    end
  end
  return A.find_spot(def_id, hx, hz, 24, 90)
end

-- Buildings: the first entry of the plan the side lacks, if it can pay; food comes first when
-- free folk run short. One site at a time. Returns what it placed or ordered.
function A.build(side_id)
  local side = S.side(side_id)
  local hold = S.hq(side_id)
  if not hold then return nil end
  for _, e in ipairs(S.structures(side_id)) do
    if not S.data(e).built then return nil end -- one site at a time
  end
  local steps = plan(side_id).build
  local free = S.food(side_id) - S.folk(side_id)
  if free < 10 then steps = { { "food", 99 } } end
  side.saving = nil
  for _, step in ipairs(steps) do
    local role, want = step[1], step[2]
    if role == "hq_up" then
      local stage = D.structure(S.data(hold).def).hq
      if stage < want then
        local hd = S.data(hold)
        for _, q in ipairs(hd.queue) do
          if q == "hq_up" then return nil end
        end
        if B.order(hold, "hq_up") then return "hq_up" end
        side.saving = "hq_up" -- save up for it
        return nil
      end
    else
      local def_id = S.role(side_id, role)
      local def = D.structure(def_id)
      if def and #S.structures(side_id, def_id) < want then
        if not Ti.allowed(side_id, def) then return nil end
        if not S.can_pay(side, def.cost) then
          side.saving = def_id
          return nil
        end
        local x, z = A.site_for(side_id, role, def_id)
        if x then return B.place(side_id, def_id, x, z) end
      end
    end
  end
  return nil
end

A.GUARD = 2 -- companies the AI keeps training even while it saves for a building

-- Training and research: at every idle building, the next unit of the plan trained there (in
-- turn), and at an idle workshop the next upgrade not yet known. While saving for the plan's
-- next building it trains only until it has A.GUARD companies.
function A.train(side_id)
  local side = S.side(side_id)
  local p = plan(side_id)
  if side.saving and #C.of_side(side_id) >= A.GUARD then return "saving" end
  side.next_kind = side.next_kind or {}
  for _, b in ipairs(S.structures(side_id, nil, true)) do
    local d = S.data(b)
    local def = D.structure(d.def)
    if #d.queue == 0 and not def.hq then
      local options = {}
      for _, id in ipairs(p.train) do
        local u = D.unit(id)
        if D.trains(u, d.def) and Ti.allowed(side_id, u) then options[#options + 1] = id end
      end
      if #options > 0 then
        local k = (side.next_kind[d.def] or 0) % #options + 1
        for i = 0, #options - 1 do
          local id = options[(k - 1 + i) % #options + 1]
          if B.order(b, id) then
            side.next_kind[d.def] = (k + i - 1) % #options + 1
            break
          end
        end
      end
      if def.workshop and #d.queue == 0 then
        for _, up in ipairs(p.upgrades or {}) do
          if not Ti.upgrade_known(side_id, up) and B.order(b, up) then break end
        end
      end
    end
  end
  -- the Wildborn raise a behemoth once they can
  if side.faction == "wildborn" and not side.behemoth then
    local throne = S.structures(side_id, "wildborn.antler_throne", true)[1]
    if throne and #S.data(throne).queue == 0 and B.order(throne, "wildborn.behemoth") then side.behemoth = true end
  end
end

-- Expansion: when the home mines run low, a new headquarters by the richest mine no headquarters
-- is near. Returns the new site, if one was placed.
function A.expand(side_id)
  local side = S.side(side_id)
  local hold = S.hq(side_id)
  if not hold or side.expanding then return nil end
  local hx, _, hz = S.pos(hold)
  local left, n = 0, 0
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) and S.data(e).kind == "mine" then
      local x, _, z = S.pos(e)
      if S.dist(x, z, hx, hz) <= A.HOME then
        left, n = left + S.data(e).gold, n + 1
      end
    end
  end
  if n > 0 and left >= n * D.MINE_GOLD * A.LOW then return nil end
  local best, most
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) and S.data(e).kind == "mine" and S.data(e).gold > D.MINE_GOLD / 2 then
      local x, _, z = S.pos(e)
      local taken = false
      for id in prts.sorted_pairs(S.match().sides) do
        for _, q in ipairs(S.hqs(id)) do
          local qx, _, qz = S.pos(q)
          if S.dist(x, z, qx, qz) < 200 then taken = true end
        end
      end
      if not taken and (not most or S.data(e).gold > most) then best, most = e, S.data(e).gold end
    end
  end
  if not best then return nil end
  local def_id = S.role(side_id, "hq")[1]
  if not S.can_pay(side, D.structure(def_id).cost) then return nil end
  local mx, _, mz = S.pos(best)
  local x, z = A.find_spot(def_id, mx, mz, 30, 90)
  if not x then return nil end
  local site = B.place(side_id, def_id, x, z)
  if site then
    side.expanding = true
    S.dispatch(side_id, "Marshal", "We ride out to hold the gold at a new headquarters.")
    prts.events.emit("rts_ai_expand", side_id, site)
  end
  return site
end

-- The army: defend the headquarters when enemies are near it; attack in waves.
function A.army(side_id, now)
  local side = S.side(side_id)
  local hold = S.hq(side_id)
  if not hold then return nil end
  local hx, _, hz = S.pos(hold)
  local companies = C.of_side(side_id)
  -- enemies seen near home?
  for _, s in ipairs(P.list(side_id, now)) do
    if s.state == "seen" and s.what == "unit" and S.dist(s.x, s.z, hx, hz) < A.DEFEND then
      for _, c in ipairs(companies) do
        if c.order.kind ~= "attack" or c.home then
          c.home = true
          C.send(c, { kind = "attack", x = s.x, z = s.z })
        end
      end
      return "defend"
    end
  end
  for _, c in ipairs(companies) do c.home = nil end
  if C.merge_remnants(side_id) > 0 then companies = C.of_side(side_id) end
  local idle, men = {}, 0
  for _, c in ipairs(companies) do
    if c.order.kind ~= "attack" and not c.broken then
      idle[#idle + 1] = c
      men = men + #c.members
    end
  end
  if men >= A.SQUAD_MEN * math.min(A.MAX_WAVE, A.FIRST_WAVE + side.wave) then
    local enemy = S.hq(side.enemy)
    if not enemy then return nil end
    local ex, _, ez = S.pos(enemy)
    side.wave = side.wave + 1
    for _, c in ipairs(idle) do C.send(c, { kind = "attack", x = ex, z = ez }) end
    S.dispatch(side_id, "Marshal", "Wave " .. side.wave .. " marches on the enemy headquarters.")
    prts.events.emit("rts_ai_attack", side_id, side.wave)
    return "attack"
  end
  -- attackers whose march is done press on against the enemy headquarters
  local enemy = S.hq(side.enemy)
  if enemy then
    local ex, _, ez = S.pos(enemy)
    for _, c in ipairs(companies) do
      if c.order.kind == "attack" and not c.home and c.order.given and now - c.order.given > 900 then
        C.send(c, { kind = "attack", x = ex, z = ez })
      end
    end
  end
  return "gather"
end

function A.think(side_id, now)
  A.economy(side_id)
  A.build(side_id)
  A.expand(side_id)
  A.train(side_id)
  return A.army(side_id, now)
end

return A
