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

function K.nearest_enemy_structure(side_id, x, z, r, weapon)
  local best, bd = nil, r
  local Bl = require("props.balloon")
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local od = S.data(e)
      -- a raised balloon is out of reach of blades
      local reachable = not (D.structure(od.def).balloon and Bl.up(e) and not (weapon and (weapon.missile or weapon.flyers)))
      if od.side ~= side_id and reachable then
        local dist = S.rect_dist(x, z, od.fp)
        if dist <= bd then best, bd = e, dist end
      end
    end
  end
  return best
end

-- Distance from (x, z) to a target (unit centre, structure footprint, ship's side).
function K.reach(x, z, t)
  local td = S.data(t)
  if td.kind == "structure" then return S.rect_dist(x, z, td.fp) end
  if td.kind == "ship" then
    local sx, _, sz = S.pos(t)
    return math.max(0, S.dist(x, z, sx, sz) - D.ship(td.def).beam)
  end
  local tx, _, tz = S.pos(t)
  return S.dist(x, z, tx, tz)
end

-- Damage to a target from a weapon. Returns true if it died or fell.
function K.hit(t, damage)
  local td = S.data(t)
  if td.kind == "structure" then return B.damage(t, damage) end
  if td.kind == "ship" then return require("rts.fleet").hit(t, damage) end
  if td.kind == "skyship" then return require("props.skyship").hit(t, damage) end
  local u = D.unit(td.def)
  td.hp = td.hp - math.max(1, damage - (u.armor or 0))
  if td.hp > 0 then return false end
  K.die(t)
  return true
end

function K.die(e)
  local d = S.data(e)
  local x, y, z = S.pos(e)
  local u = D.unit(d.def)
  if u then -- remains where they fell, for bone
    require("rts.economy").add_remains(x, z, math.min(D.REMAINS.max, D.REMAINS.per_folk * math.max(1, u.folk / u.squad)))
  end
  if prts.audio then prts.audio.play{ sound = K.SOUNDS.death, at = { x, y + 1, z }, volume = 0.6, range = 120 } end
  prts.events.emit("rts_unit_died", e, d.side, d.def)
  prts.agent.remove(e)
  prts.entity.despawn(e)
end

K.CHARGE_RUN = 15 -- ticks of riding before a strike counts as a charge

local function strike(e, d, w, target)
  d.cd = math.floor(w.cooldown * 30 + 0.5)
  local u = D.unit(d.def)
  local Ti = require("rts.tiers")
  local damage = w.damage + (u and Ti.damage_bonus(d.side, d.def) or 0)
  local x0, _, z0 = S.pos(e)
  if u and require("rts.presence").near(d.side, x0, z0) then damage = damage * require("rts.presence").STRIKE end
  local td = S.alive(target) and S.data(target)
  local now = prts.time.tick()
  if (d.weak_until or 0) > now then damage = damage * 0.5 end -- coughing in a spore cloud
  if td and td.kind == "unit" and w.vs_mounted and D.unit(td.def).mounted then damage = damage * w.vs_mounted end
  if td and td.kind == "ship" and w.ships then damage = damage * w.ships end
  if td and td.kind == "unit" and w.pin then require("rts.spells").slow(target, w.pin) end -- a harpoon pins
  if td and w.glow then -- glowstone shot: what it strikes stays lit for the side to see
    td.lit = td.lit or {}
    td.lit[d.side] = now + w.glow * 30
  end
  if td and td.kind == "structure" then
    if w.structures then damage = damage * w.structures end
    local fire = u and Ti.ignites(d.side, d.def) or 0
    if fire > 0 then require("rts.fire").ignite(target, fire) end
  end
  if u and u.charge and (d.run or 0) >= K.CHARGE_RUN then
    damage = damage * u.charge
    prts.events.emit("rts_charge", e, damage)
  end
  d.run = 0
  local x, y, z = S.pos(e)
  if prts.audio then
    prts.audio.play{ sound = w.missile and K.SOUNDS.missile or K.SOUNDS.melee, at = { x, y + 1.2, z }, volume = 0.5,
                     range = 80, pitch = w.missile and 1.4 or 1.0 }
  end
  local killed = K.hit(target, damage)
  if killed then d.target = nil end
  return killed
end

-- A company breaks when it has lost half its men; it runs home and comes back to order later.
function K.check_morale(c)
  if c.broken or not c.strength then return false end
  local cx, cz = C.centre(c)
  local Pr = require("rts.presence")
  local breaks = (cx and Pr.near(c.side, cx, cz)) and Pr.BREAK or 2
  if #C.members(c) * breaks > c.strength then return false end
  c.broken = true
  local hold = S.hq(c.side)
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
  if u.charge then d.run = prts.agent.info(e).moving and (d.run or 0) + ticks or 0 end
  local c = d.company and S.company(d.company)
  if c and not c.strength then c.strength = #c.members end
  local order = c and c.order or { kind = "idle" }
  if (d.stunned_until or 0) > prts.time.tick() then -- blinded by a Glowflare
    if prts.agent.info(e).moving then prts.agent.stop(e) end
    return "stunned"
  end
  if c and c.broken then
    if not prts.agent.info(e).moving then
      c.broken = nil
      c.strength = #C.members(c)
      C.order(c, { kind = "idle" })
    end
    return "fleeing"
  end
  if (order.kind == "move" or order.kind == "follow") and prts.agent.info(e).moving then return "marching" end
  local x, _, z = S.pos(e)
  -- an enemy commander close by is the prize
  local Su = require("rts.succession")
  local cx, cz, cdist = Su.target_for(d.side, x, z)
  if cx then
    if cdist <= w.range + 0.6 then
      if prts.agent.info(e).moving then prts.agent.stop(e) end
      if d.cd == 0 then
        d.cd = math.floor(w.cooldown * 30 + 0.5)
        Su.hurt(w.damage + require("rts.tiers").damage_bonus(d.side, d.def))
      end
      return "at the commander"
    end
    local now = prts.time.tick()
    if not d.chase or now - d.chase >= K.REPATH or not prts.agent.info(e).moving then
      d.chase = now
      prts.agent.move(e, cx, cz)
    end
    return "hunting"
  end
  -- keep a target while it lives and stays in sight; else look for one
  local t = d.target
  if t and (not S.alive(t) or K.reach(x, z, t) > u.sight * 1.3) then t = nil end
  if not t then
    local r = order.kind == "hold" and w.range + 1 or u.sight
    t = K.nearest_enemy_unit(d.side, x, z, r) or K.nearest_enemy_structure(d.side, x, z, r, w)
    if not t and w.ships then t = require("rts.fleet").enemy_ship(d.side, x, z, w.range + 6) end -- hull-borers
    if not t and w.missile then -- bows and javelins reach flyers and ships
      t = require("rts.sky").nearest_enemy(d.side, x, z, w.range) or require("rts.fleet").enemy_ship(d.side, x, z, w.range)
    end
    d.target = t
  end
  if not t then
    -- an attack-move carries on to its objective once what it met is beaten
    local now = prts.time.tick()
    if order.kind == "attack" and order.x and not prts.agent.info(e).moving and S.dist(x, z, order.x, order.z) > 10
       and now >= (d.advance_at or 0) then
      -- try again in two seconds, or in twenty if there is no way there (paths are costly)
      d.advance_at = now + (prts.agent.move(e, order.x, order.z) and 60 or 600)
      return "advancing"
    end
    return "idle"
  end
  if u.spells and order.kind == "attack" and require("rts.spells").auto(e, t) then return "casting" end
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
  local reach = def.weapon.range + def.size[1] / 2
  local t = K.nearest_enemy_unit(d.side, x, z, reach) or require("rts.sky").nearest_enemy(d.side, x, z, reach)
    or require("rts.fleet").enemy_ship(d.side, x, z, reach)
  if not t then return nil end
  strike(e, d, def.weapon, t)
  return t
end

K.enemy_of = enemy_of
return K
