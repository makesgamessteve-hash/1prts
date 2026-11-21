-- Magic (M6, concept 14): casters cast spells that cost aether from their side's stock, each with
-- its own cooldown, at targets in range that they or their side can see. Effects: Firebolt
-- (damage and fire), Wither (damage over time to the enemy's men in an area); M7 adds the Gloam's.
-- Far-sight is a ritual of a building (the Arcanum, the Spirit lodge): everything in a wide area
-- is seen once, then the ritual cools down. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local Sp = {}
Sp.EFFECTS = {}

local function now() return prts.time.tick() end

-- Whether a side sees (x, z) now: in sight of one of its eyes.
function Sp.sees(side_id, x, z)
  for _, eye in ipairs(require("rts.picture").eyes(side_id)) do
    if S.dist(eye[1], eye[2], x, z) <= eye[3] then return true end
  end
  return false
end

local function fail(caster, why)
  local d = S.data(caster)
  local u = D.unit(d.def)
  S.dispatch(d.side, u.name, why)
  prts.events.emit("rts_spell_failed", caster, why)
  return false, why
end

-- Seconds left before the caster can cast the spell again (0: ready).
function Sp.cooldown(caster, spell_id)
  local cd = S.data(caster).spell_cd
  local at = cd and cd[spell_id]
  if not at then return 0 end
  return math.max(0, (at - now()) / 30)
end

-- Casts a spell at a target entity, or at a point (x, z). Returns true, or false and why.
function Sp.cast(caster, spell_id, target, tz)
  local d = S.data(caster)
  local sp = D.spell(spell_id)
  local u = D.unit(d.def)
  if not sp or not u.spells then return false, "unknown spell" end
  local known = false
  for _, id in ipairs(u.spells) do known = known or id == spell_id end
  if not known then return false, "not my spell" end
  local x, z
  if type(target) == "number" and tz == nil then
    if not S.alive(target) then return false, "no target" end
    x, _, z = S.pos(target)
  else
    x, z = target, tz
    target = nil
  end
  if Sp.cooldown(caster, spell_id) > 0 then return fail(caster, "Not yet: the " .. sp.name .. " needs time to gather.") end
  local cx, _, cz = S.pos(caster)
  if S.dist(cx, cz, x, z) > sp.range then return fail(caster, "Too far for a " .. sp.name .. ".") end
  if not Sp.sees(d.side, x, z) then return fail(caster, "I cannot see the mark.") end
  if not S.pay(S.side(d.side), { aether = sp.aether }) then return fail(caster, "No aether left for the " .. sp.name .. ".") end
  d.spell_cd = d.spell_cd or {}
  d.spell_cd[spell_id] = now() + math.floor(sp.cooldown * 30 + 0.5)
  local effect = Sp.EFFECTS[sp.effect]
  effect(caster, sp, target, x, z)
  if prts.audio then prts.audio.play{ sound = "sounds/discover_chime.wav", at = { cx, 1.5, cz }, volume = 0.7, range = 150 } end
  prts.events.emit("rts_spell", caster, spell_id, x, z)
  return true
end

-- Firebolt: a bolt of fire; damages and sets timber alight.
function Sp.EFFECTS.firebolt(caster, sp, target, x, z)
  local K = require("rts.combat")
  local side = S.data(caster).side
  target = target or K.nearest_enemy_structure(side, x, z, 4, { missile = true }) or K.nearest_enemy_unit(side, x, z, 4)
  if not target then return nil end
  if S.data(target).kind == "structure" and D.burns(D.structure(S.data(target).def)) then
    require("rts.fire").ignite(target, sp.ignite)
  end
  K.hit(target, sp.damage)
  return target
end

-- Lingering effects (wither, the Gloam's clouds and mends): { kind, side, x, z, r, left, ... }
local function effects()
  local m = S.match()
  m.effects = m.effects or {}
  return m.effects
end

function Sp.add_effect(fx)
  local list = effects()
  list[#list + 1] = fx
  return fx
end

-- Wither: the enemy's men in the area lose health every second for a while.
function Sp.EFFECTS.wither(caster, sp, _, x, z)
  return Sp.add_effect{ kind = "wither", side = S.data(caster).side, x = x, z = z, r = sp.radius, left = sp.seconds,
                        damage = sp.damage }
end

-- Per kind of lingering effect: what it does to each unit in its area each second.
Sp.LINGER = {}
function Sp.LINGER.wither(fx, e, d)
  if d.side ~= fx.side then require("rts.combat").hit(e, fx.damage) end
end

-- Once a second: lingering effects work on the units in their areas, then run down.
function Sp.second()
  local list = effects()
  local keep = {}
  for _, fx in ipairs(list) do
    local work = Sp.LINGER[fx.kind]
    if work then
      for _, e in ipairs(prts.agent.near(fx.x, fx.z, fx.r, 64)) do
        local d = S.data(e)
        if S.alive(e) and d.kind == "unit" and d.hp > 0 then work(fx, e, d) end
      end
    end
    fx.left = fx.left - 1
    if fx.left > 0 then keep[#keep + 1] = fx end
  end
  S.match().effects = keep
  return #keep
end

-- A caster ordered to fight casts its first spell at its target on its own when it can.
function Sp.auto(caster, target)
  local u = D.unit(S.data(caster).def)
  if not u.spells or not S.alive(target) then return false end
  local id = u.spells[1]
  local sp = D.spell(id)
  if Sp.cooldown(caster, id) > 0 or S.side(S.data(caster).side).aether < sp.aether then return false end
  local tx, _, tz = S.pos(target)
  local cx, _, cz = S.pos(caster)
  if S.dist(cx, cz, tx, tz) > sp.range then return false end
  return Sp.cast(caster, id, target) == true
end

-- Far-sight: a far-sight building shows everything within a wide radius of (x, z) once.
function Sp.far_sight(structure, x, z)
  local d = S.data(structure)
  local def = D.structure(d.def)
  local F = D.FAR_SIGHT
  if not def.far_sight or not d.built then return false, "no far-sight here" end
  if (d.far_ready or 0) > now() then
    S.dispatch(d.side, def.name, "The pool is still clouded. Far-sight needs time.")
    return false, "cooling down"
  end
  if not S.pay(S.side(d.side), { aether = F.aether }) then
    S.dispatch(d.side, def.name, "Far-sight needs " .. F.aether .. " aether.")
    return false, "needs aether"
  end
  d.far_ready = now() + F.cooldown * 30
  local pic = S.side(d.side).picture
  local t = now()
  local n = 0
  for _, e in ipairs(prts.agent.near(x, z, F.radius, 256)) do
    local od = S.data(e)
    if od.kind == "unit" and od.side ~= d.side then
      local ex, _, ez = S.pos(e)
      pic[e] = { x = ex, z = ez, def = od.def, what = "unit", side = od.side, seen = t }
      n = n + 1
    end
  end
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local od = S.data(e)
      if od.side ~= d.side and S.rect_dist(x, z, od.fp) <= F.radius then
        local ex, _, ez = S.pos(e)
        pic[e] = { x = ex, z = ez, def = od.def, what = "structure", side = od.side, seen = t, built = od.built }
        n = n + 1
      end
    end
  end
  S.dispatch(d.side, def.name, "Far-sight: the pool shows " .. n .. " things of the enemy there.")
  prts.events.emit("rts_far_sight", structure, d.side, x, z, n)
  return true, n
end

prts.events.on("tick", "rts.spells", function(t)
  if t % 30 == 0 and prts.entity.find(S.REGISTRY) and S.match().running then Sp.second() end
end)
return Sp
