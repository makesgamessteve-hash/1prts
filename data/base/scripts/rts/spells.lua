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
  if not sp.sight_free and not Sp.sees(d.side, x, z) then return fail(caster, "I cannot see the mark.") end
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
    local list = fx.all and S.match().units or prts.agent.near(fx.x, fx.z, fx.r, 64)
    for _, e in ipairs(list) do
      if S.alive(e) then
        local d = S.data(e)
        local ex, _, ez = S.pos(e)
        if work and d.kind == "unit" and d.hp > 0 and S.dist(ex, ez, fx.x, fx.z) <= fx.r then work(fx, e, d) end
      end
    end
    fx.left = fx.left - 1
    if fx.left > 0 then keep[#keep + 1] = fx end
  end
  S.match().effects = keep
  local now = prts.time.tick()
  for _, e in ipairs(S.match().units) do -- out of the spores: full speed again
    if S.alive(e) then
      local d = S.data(e)
      if d.spored and (d.weak_until or 0) <= now and (d.slow_until or 0) <= now and prts.agent.info(e) then
        prts.agent.set_speed(e, d.spored)
        d.spored = nil
      end
    end
  end
  return #keep
end

-- A unit slowed to half speed for `seconds` (a harpoon's pin, spores, an undertow).
function Sp.slow(e, seconds)
  local d = S.data(e)
  d.slow_until = math.max(d.slow_until or 0, prts.time.tick() + math.floor(seconds * 30))
  local info = prts.agent.info(e)
  if info and not d.spored then
    d.spored = info.speed
    prts.agent.set_speed(e, info.speed * 0.5)
  end
  return d.slow_until
end

-- The Gloam's spells (M7). Glowflare: the enemy around the mark stands blinded (Durrow immune).
function Sp.EFFECTS.glowflare(caster, sp, _, x, z)
  local side, n = S.data(caster).side, 0
  for _, e in ipairs(prts.agent.near(x, z, sp.radius, 64)) do
    local d = S.data(e)
    if d.kind == "unit" and d.side ~= side and not D.unit(d.def).durrow then
      d.stunned_until = prts.time.tick() + sp.seconds * 30
      n = n + 1
    end
  end
  return n
end

-- Stonemeld: the gallery nearest the mark collapses, or opens again if it had collapsed.
function Sp.EFFECTS.stonemeld(_, sp, _, x, z)
  local Tu = require("rts.tunnels")
  if not Tu.exists() then return nil end
  local best, bd
  for id, g in prts.sorted_pairs(Tu.data().gals) do
    if g.box then
      local gx, gz = (g.box[1] + g.box[4]) / 2, (g.box[3] + g.box[6]) / 2
      local dist = math.max(0, math.abs(x - gx) - (g.box[4] - g.box[1]) / 2) + math.max(0, math.abs(z - gz) - (g.box[6] - g.box[3]) / 2)
      if dist <= sp.range and (not bd or dist < bd) then best, bd = id, dist end
    end
  end
  if not best then return nil end
  if Tu.data().gals[best].state == "collapsed" then Tu.open(best) else Tu.collapse(best) end
  return best
end

-- Crystal Sight: every unit in the area, in the tunnels too, is seen for a while.
function Sp.EFFECTS.crystal_sight(caster, sp, _, x, z)
  return Sp.add_effect{ kind = "crystal_sight", side = S.data(caster).side, x = x, z = z, r = sp.radius, left = sp.seconds,
                        all = true }
end
function Sp.LINGER.crystal_sight(fx, e, d)
  if d.side ~= fx.side then
    d.lit = d.lit or {}
    d.lit[fx.side] = prts.time.tick() + 45
  end
end

-- Spore Cloud: enemies in it strike at half strength and move at half speed.
function Sp.EFFECTS.spore_cloud(caster, sp, _, x, z)
  return Sp.add_effect{ kind = "spore_cloud", side = S.data(caster).side, x = x, z = z, r = sp.radius, left = sp.seconds }
end
function Sp.LINGER.spore_cloud(fx, e, d)
  if d.side == fx.side then return end
  d.weak_until = prts.time.tick() + 45
  Sp.slow(e, 1.5)
end

-- Fungal Mend: the side's units in the area heal every second, twice as fast under the ground.
function Sp.EFFECTS.fungal_mend(caster, sp, _, x, z)
  return Sp.add_effect{ kind = "fungal_mend", side = S.data(caster).side, x = x, z = z, r = sp.radius, left = sp.seconds,
                        heal = sp.heal, all = true }
end
function Sp.LINGER.fungal_mend(fx, e, d)
  if d.side ~= fx.side then return end
  local x, y, z = S.pos(e)
  local Tu = require("rts.tunnels")
  local heal = fx.heal * ((d.under or Tu.underground(x, y + 1, z)) and 2 or 1)
  d.hp = math.min(D.unit(d.def).hp, d.hp + heal)
end

-- Echo Call: every enemy within its radius, through rock too, shows up as an old sighting.
function Sp.EFFECTS.echo_call(caster, sp)
  local side = S.data(caster).side
  local cx, _, cz = S.pos(caster)
  local pic = S.side(side).picture
  local old = prts.time.tick() - require("rts.picture").SEEN_FOR - 1
  local n = 0
  for _, e in ipairs(S.match().units) do
    if S.alive(e) then
      local d = S.data(e)
      local x, _, z = S.pos(e)
      if d.side ~= side and S.dist(cx, cz, x, z) <= sp.radius then
        pic[e] = { x = x, z = z, def = d.def, what = "unit", side = d.side, seen = old }
        n = n + 1
      end
    end
  end
  return n
end

-- The League's spells (M8). Squall: a lane of wind from the caster through the mark; fires in it
-- go out, enemy flyers in it are struck and brought low, enemy ships in it are pushed along.
function Sp.EFFECTS.squall(caster, sp, _, x, z)
  local cd = S.data(caster)
  local cx, _, cz = S.pos(caster)
  local dx, dz = x - cx, z - cz
  local len = math.max(1, math.sqrt(dx * dx + dz * dz))
  local ux, uz = dx / len, dz / len
  local function in_lane(px, pz)
    local t = (px - cx) * ux + (pz - cz) * uz
    local off = math.abs((px - cx) * uz - (pz - cz) * ux)
    return t >= 0 and t <= len + sp.radius and off <= sp.radius
  end
  local n = 0
  for _, st in ipairs(S.match().structures) do
    if S.alive(st) and (S.data(st).fire or 0) > 0 then
      local px, _, pz = S.pos(st)
      if in_lane(px, pz) then S.data(st).fire = 0 n = n + 1 end
    end
  end
  for _, f in ipairs(require("rts.sky").flyers()) do
    local fd = S.data(f)
    local px, _, pz = S.pos(f)
    if fd.side ~= cd.side and in_lane(px, pz) then
      fd.low = true
      require("rts.combat").hit(f, sp.damage)
      n = n + 1
    end
  end
  for _, sh in ipairs(require("rts.fleet").ships()) do
    local sd = S.data(sh)
    local px, py, pz = S.pos(sh)
    if S.alive(sh) and sd.side ~= cd.side and in_lane(px, pz) then
      prts.entity.place(sh, { px + ux * sp.push, py, pz + uz * sp.push }, prts.entity.yaw(sh))
      n = n + 1
    end
  end
  return n
end

-- Lightning Lash: the target, then the two nearest enemies within reach of the last struck.
function Sp.EFFECTS.lightning_lash(caster, sp, target, x, z)
  local K = require("rts.combat")
  local side = S.data(caster).side
  local Wa, Storm = require("rts.water"), require("rts.storm")
  target = target or K.nearest_enemy_unit(side, x, z, 6) or K.nearest_enemy_structure(side, x, z, 6, { missile = true })
  local struck, n = {}, 0
  while target and n <= sp.jumps do
    struck[target] = true
    local tx, ty, tz = S.pos(target)
    local bonus = (Wa.depth(tx, tz) > 0 or Storm.inside(ty)) and 1.5 or 1
    local alive = S.alive(target)
    K.hit(target, sp.damage * bonus)
    n = n + 1
    local nxt
    for _, e in ipairs(prts.agent.near(tx, tz, sp.reach, 32)) do
      local ed = S.data(e)
      if not nxt and not struck[e] and ed.kind == "unit" and ed.side ~= side and ed.hp > 0 then nxt = e end
    end
    target = alive and nxt or nxt
  end
  prts.events.emit("rts_lightning_lash", caster, n)
  return n
end

-- Fog Bank: sight in the area is cut to a few metres, both ways, for a while.
function Sp.EFFECTS.fog_bank(caster, sp, _, x, z)
  return Sp.add_effect{ kind = "fog_bank", side = S.data(caster).side, x = x, z = z, r = sp.radius, left = sp.seconds,
                        sight = sp.sight }
end

-- How far one sees at (x, z) inside a fog bank, or nil outside every bank.
function Sp.fogged(x, z)
  for _, fx in ipairs(S.match().effects or {}) do
    if fx.kind == "fog_bank" and S.dist(x, z, fx.x, fx.z) <= fx.r then return fx.sight end
  end
  return nil
end

-- Tide Surge: the wave knocks the enemy's men in the area down and puts out fires there.
function Sp.EFFECTS.tide_surge(caster, sp, _, x, z)
  local side, n = S.data(caster).side, 0
  for _, e in ipairs(prts.agent.near(x, z, sp.radius, 64)) do
    local d = S.data(e)
    if d.kind == "unit" and d.side ~= side then
      d.stunned_until = prts.time.tick() + sp.seconds * 30
      n = n + 1
    end
  end
  for _, st in ipairs(S.match().structures) do
    if S.alive(st) and S.rect_dist(x, z, S.data(st).fp) <= sp.radius then S.data(st).fire = 0 end
  end
  return n
end

-- Brine Mend: one company of the side heals at once, twice as much near water.
function Sp.EFFECTS.brine_mend(caster, sp, target, x, z)
  local side = S.data(caster).side
  local C = require("rts.companies")
  local unit = target
  if not unit then
    for _, e in ipairs(prts.agent.near(x, z, 8, 32)) do
      if not unit and S.data(e).kind == "unit" and S.data(e).side == side then unit = e end
    end
  end
  if not unit or S.data(unit).side ~= side then return 0 end
  local Wa = require("rts.water")
  local c = S.data(unit).company and S.company(S.data(unit).company)
  local n = 0
  for _, e in ipairs(c and C.members(c) or { unit }) do
    local d = S.data(e)
    local ex, _, ez = S.pos(e)
    local wet = Wa.depth(ex, ez) > 0 or Wa.depth(ex + 8, ez) > 0 or Wa.depth(ex - 8, ez) > 0 or Wa.depth(ex, ez + 8) > 0
                or Wa.depth(ex, ez - 8) > 0
    d.hp = math.min(D.unit(d.def).hp, d.hp + sp.heal * (wet and 2 or 1))
    n = n + 1
  end
  return n
end

-- Undertow: the enemy's ships in the area sail at half speed for a while.
function Sp.EFFECTS.undertow(caster, sp, _, x, z)
  local side, n = S.data(caster).side, 0
  for _, sh in ipairs(require("rts.fleet").ships()) do
    local sd = S.data(sh)
    local px, _, pz = S.pos(sh)
    if S.alive(sh) and sd.side ~= side and S.dist(x, z, px, pz) <= sp.radius then
      sd.undertow = prts.time.tick() + sp.seconds * 30
      n = n + 1
    end
  end
  return n
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
  local F = { radius = def.storm_eye and D.STORM_EYE or D.FAR_SIGHT.radius, cooldown = D.FAR_SIGHT.cooldown,
              aether = D.FAR_SIGHT.aether }
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
