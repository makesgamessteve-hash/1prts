-- Fighting: every soldier thinks a few times a second. It picks the nearest enemy it can see
-- (units first, then structures), walks into range and strikes when its weapon is ready.
-- Missiles hit at once (the bolt is heard, not seen, in M2). A company whose losses pass half
-- its strength breaks and runs for its holdfast. Towers and holdfasts shoot at what comes in
-- range. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")
local C = require("rts.companies")

local K = {}
K.SOUNDS = { melee = "sounds/sword_clash.wav", missile = "sounds/crossbow_shot.wav", death = "sounds/fall_cry.wav" }
K.REPATH = 30 -- ticks between new paths towards a moving target

local function enemy_of(d, od) return od.side ~= d.side end

-- The nearest enemy unit within r of (x, z) for side.
function K.nearest_enemy_unit(side_id, x, z, r)
  for _, e in ipairs(prts.agent.near(x, z, r, 48)) do
    local od = S.data(e)
    if od.kind == "unit" and od.side ~= side_id and od.hp > 0 then return e end
  end
  return nil
end

function K.nearest_enemy_structure(side_id, x, z, r)
  local best, bd = nil, r
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local od = S.data(e)
      if od.side ~= side_id then
        local dist = S.rect_dist(x, z, od.fp)
        if dist <= bd then best, bd = e, dist end
      end
    end
  end
  return best
end

-- Distance from (x, z) to a target (unit centre or structure footprint).
function K.reach(x, z, t)
  local td = S.data(t)
  if td.kind == "structure" then return S.rect_dist(x, z, td.fp) end
  local tx, _, tz = S.pos(t)
  return S.dist(x, z, tx, tz)
end

-- Damage to a target from a weapon. Returns true if it died or fell.
function K.hit(t, damage)
  local td = S.data(t)
  if td.kind == "structure" then return B.damage(t, damage) end
  local u = D.unit(td.def)
  td.hp = td.hp - math.max(1, damage - (u.armor or 0))
  if td.hp > 0 then return false end
  K.die(t)
  return true
end

function K.die(e)
  local d = S.data(e)
  local x, y, z = S.pos(e)
  if prts.audio then prts.audio.play{ sound = K.SOUNDS.death, at = { x, y + 1, z }, volume = 0.6, range = 120 } end
  prts.events.emit("rts_unit_died", e, d.side, d.def)
  prts.agent.remove(e)
  prts.entity.despawn(e)
end

local function strike(e, d, w, target)
  d.cd = math.floor(w.cooldown * 30 + 0.5)
  local x, y, z = S.pos(e)
  if prts.audio then
    prts.audio.play{ sound = w.missile and K.SOUNDS.missile or K.SOUNDS.melee, at = { x, y + 1.2, z }, volume = 0.5,
                     range = 80, pitch = w.missile and 1.4 or 1.0 }
  end
  local killed = K.hit(target, w.damage)
  if killed then d.target = nil end
  return killed
end

-- A company breaks when it has lost half its men; it runs home and comes back to order later.
function K.check_morale(c)
  if c.broken or not c.strength then return false end
  if #C.members(c) * 2 > c.strength then return false end
  c.broken = true
  local hold = S.structures(c.side, "crown.holdfast", true)[1]
  if hold then
    local hx, _, hz = S.pos(hold)
    C.order(c, { kind = "move", x = hx, z = hz + 20 })
  end
  S.dispatch(c.side, c.name, "We are breaking! Falling back to the holdfast.")
  prts.events.emit("rts_broken", c.id)
  return true
end

-- One soldier's think (every few ticks; `ticks` since the last).
function K.think(e, ticks)
  local d = S.data(e)
  local u = D.unit(d.def)
  local w = u.weapon
  d.cd = math.max(0, d.cd - ticks)
  local c = d.company and S.company(d.company)
  if c and not c.strength then c.strength = #c.members end
  local order = c and c.order or { kind = "idle" }
  if c and c.broken then
    if not prts.agent.info(e).moving then
      c.broken = nil
      c.strength = #C.members(c)
      C.order(c, { kind = "idle" })
    end
    return "fleeing"
  end
  if order.kind == "move" and prts.agent.info(e).moving then return "marching" end
  local x, _, z = S.pos(e)
  -- keep a target while it lives and stays in sight; else look for one
  local t = d.target
  if t and (not S.alive(t) or K.reach(x, z, t) > u.sight * 1.3) then t = nil end
  if not t then
    local r = order.kind == "hold" and w.range + 1 or u.sight
    t = K.nearest_enemy_unit(d.side, x, z, r) or K.nearest_enemy_structure(d.side, x, z, r)
    d.target = t
  end
  if not t then return "idle" end
  local dist = K.reach(x, z, t)
  local reach = w.range + (S.data(t).kind == "unit" and 0.6 or 1.0)
  if dist <= reach then
    if prts.agent.info(e).moving then prts.agent.stop(e) end
    if d.cd == 0 then strike(e, d, w, t) end
    return "fighting"
  end
  if order.kind == "hold" then return "holding" end
  local now = prts.time.tick()
  if not d.chase or now - d.chase >= K.REPATH or not prts.agent.info(e).moving then
    d.chase = now
    local tx, _, tz = S.pos(t)
    local td = S.data(t)
    if td.kind == "structure" then
      tx = math.max(td.fp[1] - 1, math.min(x, td.fp[3] + 1))
      tz = math.max(td.fp[2] - 1, math.min(z, td.fp[4] + 1))
    end
    prts.agent.move(e, tx, tz)
  end
  return "closing"
end

-- Towers and holdfasts shoot at the nearest enemy unit in range.
function K.structure_think(e, ticks)
  local d = S.data(e)
  local def = D.structure(d.def)
  if not d.built or not def.weapon then return nil end
  d.cd = math.max(0, (d.cd or 0) - ticks)
  if d.cd > 0 then return nil end
  local x, _, z = S.pos(e)
  local t = K.nearest_enemy_unit(d.side, x, z, def.weapon.range + def.size[1] / 2)
  if not t then return nil end
  strike(e, d, def.weapon, t)
  return t
end

K.enemy_of = enemy_of
return K
