-- Skyships: the dwarven skyship (M5), a gondola deck under a long aether envelope with engine
-- pods, and the Crown's sky galleon (M6, tier 6), a heavy flying warship with a bridge, a gun deck
-- and a drop bay. Sent somewhere, a skyship turns and flies there at its cruising height (or at
-- the height it was sent to: up through the storm to the high sky); whoever stands on its deck
-- rides along. Sent to a citadel, it docks alongside and hands over the men it carries. Each
-- gondola is a deck plan; gaps in the rails amidships let people step across at a dock.
-- MIT License.
local Deck = require("lib.deckplan")

local Sh = {}
Sh.HEIGHT = 45
Sh.SPEED = 7
Sh.TURN = 0.2
Sh.CLIMB = 1.2   -- m/s up or down (moving platforms keep the commander below 1.6)
Sh.DOCKED = 0.8  -- how close (m) to the dock point it settles
Sh.KINDS = {
  skyship = { half_w = 3.6, half_l = 8.4, cargo = 3, levels = {
    { y = 0, height = 2.4, rows = {
      "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd",
      "dddddd", "dddddd", "#WDWW#", "#wwww#", "#wwww#", "######",
    } } }, rooms = { d = "deck", w = "wheelhouse" } },
  sky_galleon = { half_w = 4.8, half_l = 14.4, cargo = 6, hp = 2600, sight = 120,
    guns = { damage = 45, range = 70, cooldown = 5, per_side = 6 }, levels = {
    { y = 0, height = 2.6, rows = {
      "dddddddd", "dddddddd", "dddddddd", "dddddddd", "dddddddd", "dddddddd", "dddddddd", "dddddddd",
      "dddddddd", "dddddddd", "gggggggg", "gggggggg", "gggggggg", "gggggggg", "gggggggg", "gggggggg",
      "pppppppp", "pppppppp", "pppppppp", "pppppppp", "dddddddd", "#WWDWWW#", "#bbbbbb#", "#bbbbbb#",
    } } }, rooms = { d = "deck", g = "gun deck", p = "drop bay", b = "bridge" } },
}
Sh.ships = {}

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- A skyship named `name` hovering over (x, z), heading `yaw`; `kind` is "skyship" (default) or
-- "sky_galleon", `side` the side it flies for (sky galleons fight). Returns its entity.
function Sh.spawn(name, x, z, yaw, kind, side)
  kind = kind or "skyship"
  local k = Sh.KINDS[kind]
  local y = prts.scene.height(x, z) + Sh.HEIGHT
  local e = prts.entity.spawn{ name = name, pos = { x, y, z }, yaw = yaw or 0 }
  local hw, hl = k.half_w, k.half_l
  local plan = Deck.compile{ entity = e, cell = 1.2, origin = { -hw, 0, -hl }, material = "planks", wall = "oak",
                             levels = k.levels, rooms = k.rooms, prefix = name }
  local rw, rl = hw + 0.2, hl + 0.2
  box(e, "oak", -rw, -0.6, -rl, rw, -0.2, rl)                -- the gondola's hull
  for _, s in ipairs({ -1, 1 }) do                           -- rails, with a gangway gap amidships
    box(e, "oak", s > 0 and hw or -rw, -0.2, -rl, s > 0 and rw or -hw, 1.0, -1.0)
    box(e, "oak", s > 0 and hw or -rw, -0.2, 1.0, s > 0 and rw or -hw, 1.0, rl)
  end
  box(e, "oak", -rw, -0.2, -rl, rw, 1.0, -hl)
  local env_w, env_l = hw + 1.4, hl + 4.6
  for _, s in ipairs({ -1, 1 }) do -- the ropes up to the envelope, and the engine pods
    box(e, "oak", s * (hw - 0.2) - 0.05, 1.0, -hl * 0.7, s * (hw - 0.2) + 0.05, 8, -hl * 0.7 + 0.1, false)
    box(e, "oak", s * (hw - 0.2) - 0.05, 1.0, hl * 0.7, s * (hw - 0.2) + 0.05, 8, hl * 0.7 + 0.1, false)
    box(e, "iron", s * (hw + 2.9) - 0.8, 4.5, 2, s * (hw + 2.9) + 0.8, 6.0, 6, false)
  end
  box(e, "sail", -env_w, 8, -env_l, env_w, 15, env_l, false) -- the envelope
  box(e, "sail", -env_w + 1, 15, -env_l + 2, env_w - 1, 16.5, env_l - 2, false)
  box(e, "aether_glow", -1, 7.6, -4, 1, 8, 4, false)
  prts.scene.light{ pos = { 0, 2.2, hl - 3 }, color = { 0.8, 0.9, 1.0 }, radius = 9, entity = e }
  if k.guns then
    for i = 0, 2 do -- gun ports along the gun deck
      for _, s in ipairs({ -1, 1 }) do box(e, "iron", s * rw - 0.3, 0.4, -1 + i * 2.2, s * rw + 0.3, 0.9, -0.4 + i * 2.2, false) end
    end
  end
  local d = prts.entity.data(e)
  d.kind, d.plan, d.dest, d.ship_kind, d.side, d.cargo = "skyship", plan, nil, kind, side, {}
  d.hp = k.hp
  Sh.ships[#Sh.ships + 1] = name
  return e
end

function Sh.kind(e) return Sh.KINDS[prts.entity.data(e).ship_kind or "skyship"] end

-- Sends the skyship to (x, z); `alt` is an absolute height to fly at (default: cruising over the ground).
function Sh.order(e, x, z, alt)
  local d = prts.entity.data(e)
  d.dest, d.alt, d.dock, d.docked, d.alongside = { x, z }, alt, nil, nil, nil
  return true
end

-- Sends the skyship to dock alongside a citadel (by entity); it hands over its cargo there.
function Sh.dock(e, citadel)
  local d = prts.entity.data(e)
  d.dest, d.alt, d.dock, d.docked, d.alongside = nil, nil, prts.entity.name(citadel), nil, nil
  return true
end

-- Where it flies to and at what height this tick: { x, y, z } and the yaw to hold when there.
local function goal(e, d)
  if d.dock then
    local c = prts.entity.find(d.dock)
    if not c then
      d.dock = nil
      return nil
    end
    local Ci = require("rts.citadel")
    -- first to a point well off the dock, then sideways in alongside it
    local off = d.alongside and 0 or -30
    local x, y, z = Ci.world(c, Ci.DOCK[1] - (Sh.kind(e).half_w - 3.6) + off, Ci.DOCK[2], Ci.DOCK[3])
    -- a citadel still low (rising out of a lake): wait above it at a safe height
    local _, sy = prts.entity.pos(e)
    local floor = math.min(sy, prts.scene.height(x, z) + 20)
    local level = require("rts.water").level()
    if level then floor = math.max(floor, level + 20) end
    return x, math.max(y, floor), z, prts.entity.yaw(c)
  end
  if not d.dest then return nil end
  local x, z = d.dest[1], d.dest[2]
  local ground = prts.scene.height(x, z)
  local level = require("rts.water").level()
  if level and ground < level then ground = level end
  return x, d.alt or nil, z, nil
end

local function arrive_dock(e, d)
  d.docked = true
  local c = prts.entity.find(d.dock)
  local Ci = require("rts.citadel")
  local side = d.side or require("rts.command").side()
  if #d.cargo > 0 then
    Ci.receive(c, side, d.cargo)
    d.cargo = {}
  end
  prts.events.emit("skyship_docked", prts.entity.name(e), d.dock)
end

local function yaw_of(e) return prts.entity.yaw(e) end

-- One tick of flying for every skyship.
function Sh.fly(dt)
  local Storm = require("rts.storm")
  for _, name in ipairs(Sh.ships) do
    local e = prts.entity.find(name)
    local d = e and prts.entity.data(e)
    if d then
      local x, y, z = prts.entity.pos(e)
      local gx, gy, gz, gyaw = goal(e, d)
      if d.docked and gx then -- keeps station at the dock as the citadel drifts
        prts.entity.place(e, { gx, gy, gz }, gyaw)
      elseif gx then
        local dx, dz = gx - x, gz - z
        local dist = math.sqrt(dx * dx + dz * dz)
        local want_y = gy
        if not want_y then
          local ground = prts.scene.height(x, z)
          local level = require("rts.water").level()
          if level and ground < level then ground = level end
          want_y = ground + Sh.HEIGHT
        end
        if d.dock and not d.alongside and dist < 2 and math.abs(want_y - y) < 0.3 then
          d.alongside = true -- off the dock: now in sideways
          prts.entity.place(e, { x, want_y, z }, yaw_of(e))
        elseif d.dock and d.alongside then
          -- the last few metres alongside: straight in, matching the citadel's heading
          local yaw = prts.entity.yaw(e)
          local diff = (gyaw - yaw + math.pi) % (2 * math.pi) - math.pi
          yaw = yaw + math.max(-Sh.TURN * dt, math.min(Sh.TURN * dt, diff))
          local step = math.min(Sh.SPEED * 0.4 * dt, dist)
          local nx, nz = dist > 1e-6 and x + dx / dist * step or x, dist > 1e-6 and z + dz / dist * step or z
          prts.entity.place(e, { nx, want_y, nz }, yaw)
          if dist <= Sh.DOCKED and math.abs(diff) < 0.05 then arrive_dock(e, d) end
        elseif not d.dock and dist < 2 and math.abs(want_y - y) < 0.05 then
          d.dest = nil
          prts.events.emit("skyship_arrived", name)
        else
          local yaw = prts.entity.yaw(e)
          local nx, nz = x, z
          if dist >= 2 or d.dock then
            local want = math.atan(-dx, -dz)
            local diff = (want - yaw + math.pi) % (2 * math.pi) - math.pi
            yaw = yaw + math.max(-Sh.TURN * dt, math.min(Sh.TURN * dt, diff))
            -- climbing to a height far above: hold position until near it, so the climb is straight up
            local climbing = math.abs(want_y - y) > 20 and gy ~= nil
            local step = climbing and 0 or (math.abs(diff) < 0.8 and math.min(Sh.SPEED * dt, dist) or Sh.SPEED * dt * 0.3)
            nx, nz = x - math.sin(yaw) * step, z - math.cos(yaw) * step
          end
          local ny = y + math.max(-Sh.CLIMB * dt, math.min(Sh.CLIMB * dt, want_y - y))
          prts.entity.place(e, { nx, ny, nz }, yaw)
        end
      elseif Storm.inside(y) then -- no orders inside the storm: the wind takes it
        local w = Storm.wind()
        prts.entity.place(e, { x + w[1] * dt, y, z + w[2] * dt }, prts.entity.yaw(e))
      end
    end
  end
end

-- Damage to a sky galleon; at nothing left it falls burning and is gone.
function Sh.hit(e, damage)
  local d = prts.entity.data(e)
  if not d.hp then return false end
  d.hp = d.hp - damage
  if d.hp > 0 then return false end
  local x, y, z = prts.entity.pos(e)
  if prts.commander.entity() then require("rts.state").release_commander(e, 30) end
  prts.events.emit("skyship_lost", prts.entity.name(e), d.side)
  prts.entity.despawn(e)
  if prts.audio then prts.audio.play{ sound = "sounds/cannon.wav", at = { x, y, z }, range = 900 } end
  return true
end

-- Sky galleons' guns, once a second: enemy flyers first, then skyships, then structures below.
function Sh.fight(e, seconds)
  local d = prts.entity.data(e)
  local k = Sh.kind(e)
  if not k.guns or not d.side then return nil end
  d.cd = math.max(0, (d.cd or 0) - seconds)
  if d.cd > 0 then return nil end
  local S = require("rts.state")
  local x, y, z = prts.entity.pos(e)
  local g = k.guns
  local target, what
  for _, f in ipairs(require("rts.sky").flyers()) do
    local fd = S.data(f)
    local fx, fy, fz = S.pos(f)
    if fd.side ~= d.side and fd.hp > 0 and S.dist(x, z, fx, fz) <= g.range and math.abs(fy - y) <= g.range then
      target, what = f, "flyer"
      break
    end
  end
  if not target then
    for _, name in ipairs(Sh.ships) do
      local o = prts.entity.find(name)
      local od = o and prts.entity.data(o)
      if od and od.side and od.side ~= d.side and od.hp then
        local ox, _, oz = prts.entity.pos(o)
        if S.dist(x, z, ox, oz) <= g.range then target, what = o, "skyship" break end
      end
    end
  end
  if not target then
    local K = require("rts.combat")
    local st = K.nearest_enemy_structure(d.side, x, z, g.range, { missile = true })
    if st and y - prts.scene.height(x, z) <= g.range then target, what = st, "structure" end
  end
  if not target then return nil end
  for _ = 1, g.per_side do
    if not prts.entity.alive(target) then break end
    if what == "skyship" then Sh.hit(target, g.damage / g.per_side)
    else require("rts.combat").hit(target, g.damage / g.per_side) end
  end
  d.cd = g.cooldown
  if prts.audio then prts.audio.play{ sound = "sounds/cannon.wav", at = { x, y, z }, volume = 1, range = 700 } end
  prts.events.emit("skyship_broadside", prts.entity.name(e), what)
  return what
end

prts.events.on("tick", "props.skyship", function(now)
  Sh.fly(1 / 30)
  if now % 30 == 0 and prts.entity.find(require("rts.state").REGISTRY) then
    for _, name in ipairs(Sh.ships) do
      local e = prts.entity.find(name)
      if e then Sh.fight(e, 1) end
    end
  end
end)
return Sh
