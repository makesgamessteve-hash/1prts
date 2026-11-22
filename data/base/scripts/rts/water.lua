-- Water (M5, M8): a map's sea level. The water's surface is drawn over the area; where the ground
-- lies below it the water is deep. Land units keep to land (the navigation grid is closed over
-- deep water); ships keep to water deep enough for their draft. The commander wades in the
-- shallows and swims in deep water (M8): jump rises to the surface, crouch dives, walking glides,
-- and the seabed stops them. Under the surface their breath runs down (a League breath charm makes
-- it last four times as long); when it is gone they choke and are hurt. It comes back at the
-- surface and in air pockets (a diving bell, a wreck's hold). Under the surface the fog closes in
-- blue-green. MIT License.
local Wa = {}
Wa.WADE = 1.1        -- the surface swimmers tread water at, this far under it
Wa.DEEP = 0.6        -- the grid is closed where the water is deeper than this
Wa.SWIM_DEPTH = 1.5  -- deeper than this the commander swims
Wa.SLOW = 0.5        -- the commander's speeds while wading
Wa.SWIM = { glide = 1.6, dive = 1.2 }
Wa.BREATH = 30       -- seconds of breath; four times as long with a breath charm
Wa.CHARM = 4
Wa.REFILL = 10       -- seconds of breath back per second in air
Wa.CHOKE = 10        -- health lost per second without breath
Wa.FOG = { near = 1, far = 30, color = { 0.10, 0.32, 0.34 } }

local level -- nil: the map has no sea
local pockets = {}

-- cfg = { level, area = { x0, z0, x1, z1 }, fog = { near, far, colour } (the map's own) }
function Wa.setup(cfg)
  level = cfg.level
  local a = cfg.area
  prts.scene.box{ min = { a[1], level - 0.05, a[2] }, max = { a[3], level, a[4] }, material = "water", solid = false }
  Wa.area = a
  pockets = {}
  if cfg.fog then require("lib.fog").base(cfg.fog[1], cfg.fog[2], cfg.fog[3]) end
  return level
end

function Wa.level() return level end

-- How deep the water is at (x, z) (0 on land, or with no sea).
function Wa.depth(x, z)
  if not level then return 0 end
  local a = Wa.area
  if x < a[1] or x > a[3] or z < a[2] or z > a[4] then return 0 end
  return math.max(0, level - prts.scene.height(x, z))
end

-- After the navigation grid is built: deep water is closed to land units.
function Wa.close_grid()
  if not level then return 0 end
  return prts.nav.block_below(level - Wa.DEEP)
end

-- An air pocket: a box (min, max) in an entity's frame where there is air under the water.
function Wa.air(e, min, max)
  pockets[#pockets + 1] = { name = prts.entity.name(e), min = min, max = max }
  return #pockets
end

-- Whether a world point is in an air pocket.
function Wa.in_air(x, y, z)
  local Deck = require("lib.deckplan")
  for _, p in ipairs(pockets) do
    local e = prts.entity.find(p.name)
    if e then
      local lx, ly, lz = Deck.to_frame(e, x, y, z)
      if lx >= p.min[1] and lx <= p.max[1] and ly >= p.min[2] and ly <= p.max[2] and lz >= p.min[3] and lz <= p.max[3] then
        return true
      end
    end
  end
  return false
end

-- Whether a world point is under the water (and not in air).
function Wa.under(x, y, z)
  return level ~= nil and y < level and Wa.depth(x, z) > 0 and not Wa.in_air(x, y, z)
end

-- How many seconds of breath the commander has in all (a League breath charm: four times more).
function Wa.lungs()
  local Cmd = require("rts.command")
  local S = require("rts.state")
  local side = prts.entity.find(S.REGISTRY) and Cmd.side() and S.side(Cmd.side())
  if side and (side.faction == "brine" or (side.upgrades and side.upgrades["brine.breath_charm"])) then
    return Wa.BREATH * Wa.CHARM
  end
  return Wa.BREATH
end

-- The commander's breath now (seconds) and in all.
function Wa.breath()
  local st = prts.entity.find("commander.breath")
  local full = Wa.lungs()
  if not st then return full, full end
  return prts.entity.data(st).left, full
end

-- The commander's state while in deep water (swimming), held as entity data.
local function swim(c, x, y, z, dt)
  local st = prts.entity.find("commander.swimming")
  if not st then
    st = prts.entity.spawn{ name = "commander.swimming" }
    prts.entity.data(st).y = y
    prts.events.emit("swimming", true)
  end
  local d = prts.entity.data(st)
  local I = prts.input
  local up = (I.down("jump") and 1 or 0) - (I.down("crouch") and 1 or 0)
  local fwd = I.value("move_forward") - I.value("move_back")
  local side = I.value("move_right") - I.value("move_left")
  local view = prts.commander.view()
  local speed = up < 0 and Wa.SWIM.dive or Wa.SWIM.glide
  local nx = x + (-math.sin(view) * fwd + math.cos(view) * side) * Wa.SWIM.glide * dt
  local nz = z + (-math.cos(view) * fwd - math.sin(view) * side) * Wa.SWIM.glide * dt
  local ny = (d.y or y) + up * speed * dt
  if Wa.depth(nx, nz) < Wa.SWIM_DEPTH then nx, nz = x, z end -- the shallows: wade out on your feet instead
  ny = math.min(level - Wa.WADE, math.max(prts.scene.height(nx, nz), ny))
  d.y = ny
  prts.commander.place({ nx, ny, nz })
  return ny
end

local function wade(wading)
  local st = prts.entity.find("commander.wading")
  if wading and not st then
    st = prts.entity.spawn{ name = "commander.wading" }
    local now = prts.commander.tune()
    local d = prts.entity.data(st)
    d.saved = { walk_speed = now.walk_speed, run_speed = now.run_speed, sprint_speed = now.sprint_speed }
    prts.commander.tune{ walk_speed = now.walk_speed * Wa.SLOW, run_speed = now.run_speed * Wa.SLOW,
                         sprint_speed = now.sprint_speed * Wa.SLOW }
    prts.events.emit("wading", true)
  elseif not wading and st then
    prts.commander.tune(prts.entity.data(st).saved)
    prts.entity.despawn(st)
    prts.events.emit("wading", false)
  end
end

-- Breath runs down under the water, comes back in air; without it the commander chokes.
local function breathe(under, dt)
  local full = Wa.lungs()
  local st = prts.entity.find("commander.breath")
  if not st then
    if not under then return full end
    st = prts.entity.spawn{ name = "commander.breath" }
    prts.entity.data(st).left = full
  end
  local d = prts.entity.data(st)
  if under then
    d.left = math.max(0, d.left - dt)
    if d.left <= 0 then
      d.choke = (d.choke or 0) + dt
      if d.choke >= 1 then
        d.choke = d.choke - 1
        prts.events.emit("choking")
        if prts.entity.find(require("rts.state").REGISTRY) then require("rts.succession").hurt(Wa.CHOKE) end
      end
    end
  else
    d.left = math.min(full, d.left + Wa.REFILL * dt)
    d.choke = 0
    if d.left >= full then prts.entity.despawn(st) end
  end
  return d.left
end

-- Every tick: wading in the shallows, swimming in deep water, breath and the underwater fog.
function Wa.tick()
  local c = prts.commander.entity()
  if not level or not c then return nil end
  local dt = 1 / 30
  local x, y, z = prts.entity.pos(c)
  local depth = Wa.depth(x, z)
  local in_air = Wa.in_air(x, y + 1, z)
  local swimming = depth >= Wa.SWIM_DEPTH and y < level - 0.2 and not in_air
  if swimming then
    wade(false)
    y = swim(c, x, y, z, dt)
  else
    local st = prts.entity.find("commander.swimming")
    if st then
      prts.entity.despawn(st)
      prts.events.emit("swimming", false)
    end
    wade(depth > 0.4 and y < level - 0.2 and not in_air)
  end
  local _, _, eye = prts.commander.view()
  local under = Wa.under(x, y + eye, z)
  local Fog = require("lib.fog")
  local was = Fog.set("underwater", under and Wa.FOG.near or nil, Wa.FOG.far, Wa.FOG.color)
  if was then prts.events.emit("underwater", under) end
  breathe(under, dt)
  return swimming or depth > 0.4
end

prts.events.on("tick", "rts.water", Wa.tick)
return Wa
