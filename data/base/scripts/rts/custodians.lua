-- The Custodians (M10): the guardians of the floating citadels wake with them. They are a fifth
-- side without a commander: no headquarters, no workers, and they never win or lose a match.
-- Each construct keeps to the ward round its post: neutral Custodians fight only what comes in,
-- hostile ones also raid the nearest headquarters now and then. They "repair" the land the way
-- the Imperium wanted it: a spire reclaims the aether wells near it (they give their owners
-- nothing), and an uncontested ruin of the Imperium is rebuilt into a new spire. Aboard a
-- citadel they are part of its garrison (rts/citadel.lua) and stand in its courtyard and core.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local K = require("rts.combat")

local Cu = {}
Cu.SIDE = 9
Cu.WARD = 45       -- metres round a construct's post that it keeps
Cu.HOME = 8        -- farther than this from its post with its ward clear, a construct walks back
Cu.RAID = 180      -- seconds between a hostile side's raids
Cu.RECLAIM = 40    -- metres round a spire within which aether wells are reclaimed
Cu.REBUILD = 60    -- seconds a ruin must be held, uncontested, to rise as a spire
Cu.CLEAR = 30      -- metres round a ruin that must be free of enemies while it is rebuilt
Cu.REFORM = 120    -- seconds for a spire to form a new warden while its guard is short
Cu.GUARD = { "custodian.warden", "custodian.warden", "custodian.lancer", "custodian.mote" }
-- where the guardians stand aboard a citadel (its frame): the courtyard, and the core
Cu.ABOARD = { { -2.5, -10, 3 }, { 2.5, -10, 3 }, { -8, 0, -8 }, { 8, 0, -8 }, { -8, 0, 6 }, { 8, 0, 6 },
              { -12, 0, -2 }, { 12, 0, -2 }, { -3, -10, 0 }, { 3, -10, 6 } }

local function state() return S.match().custodians end

-- Whether Custodians take part in this match.
function Cu.active() return state() ~= nil end

-- The Custodians join a match as side Cu.SIDE: stance "neutral" or "hostile", and the Imperium
-- ruins they hold ({ { x, z } }). Returns the side.
function Cu.join(stance, ruins)
  local m = S.match()
  local side = S.add_side{ id = Cu.SIDE, name = "The Custodians", faction = "custodians", color = "custodian" }
  side.custodians, side.stance = true, stance or "neutral"
  m.custodians = { ruins = {}, raid_at = prts.time.tick() + Cu.RAID * 30, aboard = {} }
  for _, r in ipairs(ruins or {}) do Cu.ruin(r.x, r.z, side.stance == "hostile") end
  prts.events.emit("custodians_joined", side.stance)
  return side
end

-- A broken ring of the Imperium's stones at (x, z), with a guard woken at it (and a spire if
-- `spire`). Returns the ruin's index.
function Cu.ruin(x, z, spire)
  local st = state()
  local i = #st.ruins + 1
  local y = prts.scene.height(x, z)
  for k = 0, 6 do
    local a = k * math.pi * 2 / 7
    local px, pz = x + math.cos(a) * 9, z + math.sin(a) * 9
    local h = 1.2 + (k * 37 % 5) * 0.5
    prts.scene.box{ min = { px - 0.6, y - 0.5, pz - 0.6 }, max = { px + 0.6, y + h, pz + 0.6 }, material = "stone",
                      solid = false }
  end
  prts.scene.marker("ruin." .. i, { x, y, z }, 0)
  local ruin = { x = x, z = z, held = 0, reform = 0 }
  st.ruins[i] = ruin
  ruin.company = Cu.wake(x, z, "Guard of ruin " .. i).id
  if spire then ruin.spire = require("rts.structures").place(Cu.SIDE, "custodian.spire", x, z, true) end
  return i
end

-- Wakes a guard at (x, z): constructs of `list` (default Cu.GUARD) posted round the point, in one
-- company. Returns the company.
function Cu.wake(x, z, label, list)
  local c = S.new_company(Cu.SIDE, label or "Guard")
  for i, def in ipairs(list or Cu.GUARD) do
    local a = i * math.pi * 2 / #(list or Cu.GUARD)
    local px, pz = x + math.cos(a) * 4, z + math.sin(a) * 4
    local e = S.spawn_unit(Cu.SIDE, def, px, pz, c)
    S.data(e).post = { px, pz }
  end
  prts.events.emit("custodians_woken", x, z, #c.members)
  return c
end

-- Guardians aboard a citadel: a garrison of constructs (the citadel's fights count them) and
-- their likenesses standing in its courtyard and core, which ride along with it.
function Cu.wake_aboard(citadel, list)
  local st = state()
  local name = prts.entity.name(citadel)
  if st.aboard[name] then return 0 end
  list = list or { "custodian.warden", "custodian.warden", "custodian.warden", "custodian.lancer",
                   "custodian.warden", "custodian.lancer" }
  local c = S.new_company(Cu.SIDE, "Guardians of " .. name)
  local rec = { id = c.id, units = {} }
  local figures = {}
  for i, def in ipairs(list) do
    rec.units[i] = { def = def, hp = D.unit(def).hp }
    local p = Cu.ABOARD[(i - 1) % #Cu.ABOARD + 1]
    local f = prts.entity.spawn{ name = name .. ".custodian." .. i, parent = citadel, pos = p }
    -- (a likeness at most 4 m tall: the core's vault is low)
    prts.scene.model{ path = D.unit(def).model, entity = f, pos = { 0, 0, 0 }, scale = math.min(0.1, D.unit(def).voxel) }
    figures[i] = prts.entity.name(f)
  end
  st.aboard[name] = { figures = figures, company = c.id }
  require("rts.citadel").receive(citadel, Cu.SIDE, { rec })
  prts.events.emit("custodians_woken_aboard", citadel, #list)
  return #list
end

-- The likenesses aboard follow the garrison: as Custodians fall aboard, figures go.
local function sync_aboard()
  local Ci = require("rts.citadel")
  for name, a in prts.sorted_pairs(state().aboard) do
    local cit = prts.entity.find(name)
    local men = cit and Ci.strength(cit, Cu.SIDE) or 0
    for i = #a.figures, men + 1, -1 do
      local f = prts.entity.find(a.figures[i])
      if f then prts.entity.despawn(f) end
      a.figures[i] = nil
    end
  end
end

-- Constructs keep to their wards: with no enemy inside, one away from its post walks back; one
-- chasing a foe out of its ward lets it go. Raiders are off the leash.
function Cu.leash()
  local n = 0
  for _, e in ipairs(S.units(Cu.SIDE)) do
    local d = S.data(e)
    local c = d.company and S.company(d.company)
    if d.post and not d.flying and not (c and c.order.kind == "attack") then
      local x, _, z = S.pos(e)
      local px, pz = d.post[1], d.post[2]
      local foe = K.nearest_enemy_unit(Cu.SIDE, px, pz, Cu.WARD)
      if d.target and S.alive(d.target) then
        local tx, _, tz = S.pos(d.target)
        if S.dist(tx, tz, px, pz) > Cu.WARD + 10 then d.target, foe = nil, nil end
      end
      if not foe and S.dist(x, z, px, pz) > Cu.HOME and not prts.agent.info(e).moving then
        d.target = nil
        prts.agent.move(e, px, pz)
        n = n + 1
      end
    end
  end
  return n
end

-- A hostile side's raid: every ruin's guard marches on the headquarters nearest to its ruin.
function Cu.raid()
  local st = state()
  local n = 0
  for _, ruin in ipairs(st.ruins) do
    local c = ruin.company and S.company(ruin.company)
    local best, bd
    for id, side in prts.sorted_pairs(S.match().sides) do
      if id ~= Cu.SIDE and not side.lost then
        for _, hq in ipairs(S.hqs(id)) do
          local hx, _, hz = S.pos(hq)
          local dist = S.dist(ruin.x, ruin.z, hx, hz)
          if not bd or dist < bd then best, bd = hq, dist end
        end
      end
    end
    if c and best and #require("rts.companies").members(c) > 0 then
      local hx, _, hz = S.pos(best)
      require("rts.companies").order(c, { kind = "attack", x = hx, z = hz })
      n = n + 1
    end
  end
  if n > 0 then prts.events.emit("custodians_raid", n) end
  return n
end

-- Spires reclaim the aether wells (and shrines) of other sides near them.
function Cu.reclaim()
  local spires = S.structures(Cu.SIDE, "custodian.spire", true)
  local n = 0
  for _, e in ipairs(S.match().structures) do
    local d = S.alive(e) and S.data(e)
    if d and d.side ~= Cu.SIDE and D.structure(d.def).aether then
      local x, _, z = S.pos(e)
      local near = false
      for _, sp in ipairs(spires) do
        local sx, _, sz = S.pos(sp)
        if S.dist(x, z, sx, sz) <= Cu.RECLAIM then near = true end
      end
      if near and not d.reclaimed then
        d.reclaimed = prts.time.tick()
        S.dispatch(d.side, "Steward", "The Custodians have taken the well. It gives us nothing.")
        prts.events.emit("custodians_reclaimed", e)
      elseif not near and d.reclaimed then
        d.reclaimed = nil
        S.dispatch(d.side, "Steward", "The well is ours again.")
      end
      if d.reclaimed then n = n + 1 end
    end
  end
  return n
end

-- Ruins held by Custodians with no enemy near rise as spires; spires re-form wardens.
function Cu.rebuild(seconds)
  local St = require("rts.structures")
  local C = require("rts.companies")
  local risen = 0
  for _, ruin in ipairs(state().ruins) do
    local c = ruin.company and S.company(ruin.company)
    local guard = c and #C.members(c) or 0
    if ruin.spire and S.alive(ruin.spire) then
      ruin.reform = ruin.reform + seconds
      if guard < #Cu.GUARD and ruin.reform >= Cu.REFORM then
        ruin.reform = 0
        local e = S.spawn_unit(Cu.SIDE, "custodian.warden", ruin.x + 6, ruin.z, c)
        S.data(e).post = { ruin.x + 6, ruin.z }
      end
    else
      ruin.spire = nil
      local here = 0
      for _, e in ipairs(S.units(Cu.SIDE)) do
        local x, _, z = S.pos(e)
        if S.dist(x, z, ruin.x, ruin.z) <= 20 then here = here + 1 end
      end
      if here > 0 and not K.nearest_enemy_unit(Cu.SIDE, ruin.x, ruin.z, Cu.CLEAR) then
        ruin.held = ruin.held + seconds
      else
        ruin.held = 0
      end
      if ruin.held >= Cu.REBUILD then
        ruin.held = 0
        ruin.spire = St.place(Cu.SIDE, "custodian.spire", ruin.x, ruin.z, true)
        prts.events.emit("custodians_rebuilt", ruin.spire)
        risen = risen + 1
      end
    end
  end
  return risen
end

-- Every second while Custodians take part.
function Cu.tick(now)
  if not prts.entity.find(S.REGISTRY) or not Cu.active() or not S.match().running then return false end
  if now % 30 ~= 0 then return false end
  local st = state()
  Cu.leash()
  Cu.reclaim()
  Cu.rebuild(1)
  sync_aboard()
  if S.side(Cu.SIDE).stance == "hostile" and now >= st.raid_at then
    st.raid_at = now + Cu.RAID * 30
    Cu.raid()
  end
  return true
end

-- A citadel woken anywhere wakes the guardians of every other citadel on the map.
function Cu.on_citadel_woken(e)
  if not Cu.active() then return 0 end
  local n = 0
  for _, other in ipairs(S.match().citadels or {}) do
    if other ~= e and S.alive(other) and S.data(other).state ~= "broken" then n = n + Cu.wake_aboard(other) end
  end
  return n
end

prts.events.on("tick", "rts.custodians", Cu.tick)
prts.events.on("citadel_woken", "rts.custodians", Cu.on_citadel_woken)
return Cu
