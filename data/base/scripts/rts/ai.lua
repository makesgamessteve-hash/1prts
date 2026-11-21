-- The opposing commander: a plain build order and attack waves. Every second it keeps serfs
-- working (two thirds on gold, a third on timber), adds farmsteads before folk runs out, raises
-- a barracks and a watchtower, trains foot and bows in turn, defends its holdfast and sends its
-- army at the enemy holdfast in growing waves. It uses the same rules and the same sending
-- stones as the player. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")
local E = require("rts.economy")
local C = require("rts.companies")
local P = require("rts.picture")

local A = {}
A.SERFS = 12
A.FIRST_WAVE = 2   -- companies in the first attack; each wave one more, up to A.MAX_WAVE
A.MAX_WAVE = 6
A.SQUAD_MEN = 8    -- men counted as one company when sizing a wave
A.DEFEND = 70      -- metres around the holdfast that the AI defends

local function holdfast(side_id) return S.structures(side_id, "crown.holdfast", true)[1] end

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

-- Serfs: train up to A.SERFS; idle serfs gather (gold 2 : timber 1) or build open sites.
function A.economy(side_id)
  local side = S.side(side_id)
  local hold = holdfast(side_id)
  if not hold then return end
  local serfs = S.units(side_id, "crown.serf")
  local hd = S.data(hold)
  if #serfs + #hd.queue < A.SERFS and #hd.queue == 0 then B.order_unit(hold, "crown.serf") end
  local sites = {}
  for _, e in ipairs(S.structures(side_id)) do
    if not S.data(e).built then sites[#sites + 1] = e end
  end
  local builders = count(serfs, function(e) local t = S.data(e).task return t and t.t == "build" end)
  -- a site without enough builders takes serfs off gathering (not while they carry a load)
  for _, e in ipairs(serfs) do
    if #sites == 0 or builders >= 2 then break end
    local d = S.data(e)
    if d.task and d.task.t == "gather" and not d.carry then
      E.build(e, sites[1])
      builders = builders + 1
    end
  end
  local gold = count(serfs, function(e) local t = S.data(e).task return t and t.res == "gold" end)
  -- plenty of timber: an empty-handed woodcutter goes to the mines instead
  if side.timber > side.gold + 300 then
    for _, e in ipairs(serfs) do
      local d = S.data(e)
      if d.task and d.task.t == "gather" and d.task.res == "timber" and not d.carry then
        if E.gather(e, "gold") then gold = gold + 1 end
        break
      end
    end
  end
  for _, e in ipairs(serfs) do
    local d = S.data(e)
    if not d.task then
      if #sites > 0 and builders < 2 then
        E.build(e, sites[1])
        builders = builders + 1
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

local function want(side_id, def_id) return #S.structures(side_id, def_id) end

-- Buildings: farmsteads ahead of folk, then a barracks, a watchtower, a second barracks.
function A.build(side_id)
  local side = S.side(side_id)
  local hold = holdfast(side_id)
  if not hold then return nil end
  local hx, _, hz = S.pos(hold)
  for _, e in ipairs(S.structures(side_id)) do
    if not S.data(e).built then return nil end -- one site at a time
  end
  local plan
  local free = S.food(side_id) - S.folk(side_id)
  if free < 10 then plan = "crown.farmstead"
  elseif want(side_id, "crown.barracks") == 0 then plan = "crown.barracks"
  elseif want(side_id, "crown.watchtower") == 0 then plan = "crown.watchtower"
  elseif want(side_id, "crown.barracks") < 2 and side.gold > 400 then plan = "crown.barracks"
  end
  if not plan or not S.can_pay(side, D.structure(plan).cost) then return nil end
  local x, z
  if plan == "crown.watchtower" then
    local enemy = holdfast(side.enemy)
    if enemy then
      local ex, _, ez = S.pos(enemy)
      local dx, dz = ex - hx, ez - hz
      local len = math.sqrt(dx * dx + dz * dz)
      x, z = A.find_spot(plan, hx + dx / len * 90, hz + dz / len * 90, 0, 40)
    end
  else
    x, z = A.find_spot(plan, hx, hz, 24, 80)
  end
  if not x then return nil end
  return B.place(side_id, plan, x, z)
end

-- Training: foot and bows in turn at every idle barracks.
function A.train(side_id)
  local side = S.side(side_id)
  for _, b in ipairs(S.structures(side_id, "crown.barracks", true)) do
    local d = S.data(b)
    if #d.queue == 0 then
      side.next_kind = side.next_kind == "crown.men_at_arms" and "crown.crossbowmen" or "crown.men_at_arms"
      if not B.order_unit(b, side.next_kind) then
        -- the other kind, if that one can be paid for
        local other = side.next_kind == "crown.men_at_arms" and "crown.crossbowmen" or "crown.men_at_arms"
        if B.order_unit(b, other) then side.next_kind = other end
      end
    end
  end
end

-- The army: defend the holdfast when enemies are near it; attack in waves.
function A.army(side_id, now)
  local side = S.side(side_id)
  local hold = holdfast(side_id)
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
    local enemy = holdfast(side.enemy)
    if not enemy then return nil end
    local ex, _, ez = S.pos(enemy)
    side.wave = side.wave + 1
    for _, c in ipairs(idle) do C.send(c, { kind = "attack", x = ex, z = ez }) end
    S.dispatch(side_id, "Marshal", "Wave " .. side.wave .. " marches on the enemy holdfast.")
    prts.events.emit("rts_ai_attack", side_id, side.wave)
    return "attack"
  end
  -- attackers whose march is done press on against the nearest enemy structure
  local enemy = holdfast(side.enemy)
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
  A.train(side_id)
  return A.army(side_id, now)
end

return A
