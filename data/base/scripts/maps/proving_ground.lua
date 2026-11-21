-- The Proving Ground: a test map with a three-storey keep (switchback stairs, a hoist lift
-- and a war room with a placeholder war table) in open country at dusk. MIT License.
local B = require("lib.build")
local Door = require("props.door")
local Lift = require("props.lift")

local M = {}

-- Keep dimensions (meters). The keep is centred on the origin, its door faces +z (south).
M.HX, M.HZ, M.T = 12, 10, 1.0
M.FLOORS = { 0.2, 4.7, 9.2 }  -- walking height of each storey
M.SLAB = 0.3
M.ROOF = 13.7
M.LIFT = { x = -9.3, z = 0, gate = -7.4 }

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.86, 0.95, 0.80 }, uv_scale = 4 }
  m{ id = "dirt", texture = "textures/dirt.png", uv_scale = 3 }
  m{ id = "stone", texture = "textures/stone_wall.png", uv_scale = 3 }
  m{ id = "stone_floor", texture = "textures/stone_floor.png", uv_scale = 3 }
  m{ id = "planks", texture = "textures/wood_planks.png", uv_scale = 2 }
  m{ id = "oak", texture = "textures/wood_dark.png", uv_scale = 2.2 }
  m{ id = "plaster", texture = "textures/plaster.png", uv_scale = 3 }
  m{ id = "slate", texture = "textures/roof_slate.png", uv_scale = 2 }
  m{ id = "iron", texture = "textures/iron.png", uv_scale = 1 }
  m{ id = "gate", texture = "textures/gate_iron.png", uv_scale = 1.4 }
  m{ id = "banner", texture = "textures/banner_crown.png", uv_scale = 1.6 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "rug", texture = "textures/rug.png", uv_scale = 2.6 }
  m{ id = "bark", texture = "textures/bark.png", uv_scale = 1 }
  m{ id = "pine", texture = "textures/pine.png", uv_scale = 2 }
  m{ id = "flame", texture = "textures/flame.png", uv_scale = 0.3, emissive = true }
  m{ id = "rock", texture = "textures/rock.png", uv_scale = 2 }
end

local function torch(x, y, z, facing_x, facing_z)
  B.box("iron", x - 0.06 - facing_x * 0.1, y - 0.3, z - 0.06 - facing_z * 0.1,
        x + 0.06 - facing_x * 0.1, y, z + 0.06 - facing_z * 0.1, { solid = false })
  B.box("flame", x - 0.07, y, z - 0.07, x + 0.07, y + 0.22, z + 0.07, { solid = false })
  prts.scene.light{ pos = { x + facing_x * 0.4, y + 0.3, z + facing_z * 0.4 }, color = { 1.35, 0.82, 0.42 }, radius = 11 }
end

-- A looping ambience sound at a fixed place (the forge, torches).
local function ambience(sound, x, y, z, volume, range)
  return prts.audio.play{ sound = sound, at = { x, y, z }, bus = "ambience", loop = true, volume = volume, range = range }
end

-- An upper floor, leaving holes for the stairwell (north-east) and the lift shaft (west).
local function slab(y_top)
  local y0 = y_top - M.SLAB
  B.box("planks", -7.6, y0, -3, 11, y_top, 9)          -- main part (east of the lift shaft)
  B.box("planks", -11, y0, -9, 6, y_top, -3)           -- north part (west of the stairwell)
  B.box("planks", -11, y0, 1.9, -7.6, y_top, 9)        -- south of the lift shaft
  B.box("planks", -11, y0, -3, -7.6, y_top, -1.9)      -- north of the lift shaft
end

local function outer_walls()
  local HX, HZ, T, F = M.HX, M.HZ, M.T, M.FLOORS
  local function windows(at)
    local w = {}
    for f = 2, 3 do
      for _, c in ipairs(at) do
        w[#w + 1] = { from = c - 0.4, to = c + 0.4, bottom = F[f] + 1.1, top = F[f] + 2.7 }
      end
    end
    return w
  end
  -- south wall with the main door
  local south = windows({ -7, -3.5, 3.5, 7 })
  south[#south + 1] = { from = -1.1, to = 1.1, bottom = 0.2, top = 3.4 }
  B.wall("stone", "x", -HX, HX, HZ - T, HZ, 0, M.ROOF, south)
  B.wall("stone", "x", -HX, HX, -HZ, -HZ + T, 0, M.ROOF, windows({ -7, -2, 2, 7 }))
  B.wall("stone", "z", -HZ + T, HZ - T, HX - T, HX, 0, M.ROOF, windows({ 2, 6 }))
  B.wall("stone", "z", -HZ + T, HZ - T, -HX, -HX + T, 0, M.ROOF, windows({ -6, 6 }))
end

local function stairwell()
  local F = M.FLOORS
  for f = 1, 3 do torch(10.9, F[f] + 2.4, -2.0, -1, 0) end
  torch(8.0, F[1] + 4.6, -8.9, 0, 1)
  torch(8.0, F[2] + 4.6, -8.9, 0, 1)
  -- inner wall between the great hall and the stairwell, and the wall between flights
  B.box("plaster", 5.4, 0, -9, 6, M.ROOF - M.SLAB, -1)
  B.box("plaster", 8.4, 0, -5.7, 8.6, M.ROOF - M.SLAB, -3)
  for f = 1, 2 do
    local base = F[f]
    local mid = B.stairs("stone_floor", { axis = "z", sign = -1, start = -3, w0 = 8.6, w1 = 11, steps = 9,
                                          rise = 0.25, run = 0.3, base = base, solid = f == 1 })
    B.box("stone_floor", 6, mid - 0.3, -9, 11, mid, -5.7)  -- landing
    B.stairs("stone_floor", { axis = "z", sign = 1, start = -5.7, w0 = 6, w1 = 8.4, steps = 9,
                              rise = 0.25, run = 0.3, base = mid })
  end
end

local function lift_shaft()
  local F, L = M.FLOORS, M.LIFT
  B.box("plaster", -11, 0, -2.3, -7.6, M.ROOF - M.SLAB, -1.9)
  B.box("plaster", -11, 0, 1.9, -7.6, M.ROOF - M.SLAB, 2.3)
  local gates = {}
  for f = 1, 3 do gates[f] = { from = -1.2, to = 1.2, bottom = F[f], top = F[f] + 2.8 } end
  B.wall("plaster", "z", -2.3, 2.3, -7.6, -7.2, 0, M.ROOF - M.SLAB, gates)
  B.box("stone_floor", -11, -0.6, -1.9, -7.6, -0.3, 1.9)  -- shaft pit floor
  return Lift.make{ name = "keep.lift", pos = { L.x, F[1], L.z }, floors = F, size = { 3.2, 3.6 },
                    gate = L.gate, material = "planks", gate_material = "gate", lever_material = "iron" }
end

local function great_hall()
  local F = M.FLOORS
  for _, x in ipairs({ -4, 2 }) do
    for _, z in ipairs({ -4, 1, 6 }) do B.box("stone", x - 0.35, F[1], z - 0.35, x + 0.35, F[2] - M.SLAB, z + 0.35) end
  end
  B.box("table", -3, F[1] + 0.75, 2.6, 1, F[1] + 0.85, 4.0)          -- long table
  B.box("oak", -2.9, F[1], 2.7, -2.7, F[1] + 0.75, 3.9)
  B.box("oak", 0.7, F[1], 2.7, 0.9, F[1] + 0.75, 3.9)
  B.box("oak", -3, F[1], 1.9, 1, F[1] + 0.45, 2.3)                   -- benches
  B.box("oak", -3, F[1], 4.3, 1, F[1] + 0.45, 4.7)
  B.box("rug", -2.5, F[1], -2, 1.5, F[1] + 0.02, 8.5, { solid = false })
  B.box("banner", -2.0, F[1] + 1.2, -8.98, 0.0, F[1] + 3.8, -8.92, { solid = false })
  B.box("banner", 2.5, F[1] + 1.2, -8.98, 4.5, F[1] + 3.8, -8.92, { solid = false })
  torch(-6.0, F[1] + 2.4, -8.9, 0, 1)
  torch(3.5, F[1] + 2.4, -8.9, 0, 1)
  torch(5.0, F[1] + 2.4, 8.9, 0, -1)
  torch(5.3, F[1] + 2.4, 2.0, -1, 0)
  B.box("stone", -1.6, F[1], -8.9, 1.6, F[1] + 1.1, -7.6)            -- hall hearth
  B.box("flame", -1.2, F[1] + 0.3, -8.5, 1.2, F[1] + 0.8, -7.9, { solid = false })
  prts.scene.light{ pos = { 0, F[1] + 1.4, -7.0 }, color = { 1.6, 0.75, 0.3 }, radius = 12 }
  torch(-10.9, F[1] + 2.4, 6.0, 1, 0)
  torch(-1.6, F[1] + 2.4, 8.9, 0, -1)
  M.torch_sounds = { ambience("sounds/torch_loop.wav", -1.6, F[1] + 2.5, 8.9, 0.5, 12),
                     ambience("sounds/torch_loop.wav", 5.0, F[1] + 2.5, 8.9, 0.5, 12) }
  prts.scene.zone("keep.great_hall", { -11, F[1] - 0.1, -9 }, { 11, F[2] - M.SLAB, 9 })
end

local function forge_room()
  local F = M.FLOORS
  B.wall("plaster", "x", -11, -4.9, -4.3, -3.9, F[1] - 0.2, F[2] - M.SLAB,
         { { from = -9.0, to = -7.8, bottom = F[1], top = F[1] + 2.4 } })
  B.box("plaster", -5.3, F[1] - 0.2, -9, -4.9, F[2] - M.SLAB, -4.3)
  Door.make{ name = "keep.forge.door", hinge = { -9.0, F[1], -4.1 }, width = 1.2, height = 2.4, material = "oak" }
  B.box("stone", -10.9, F[1], -8.9, -8.5, F[1] + 1.0, -7.4)          -- hearth
  B.box("flame", -10.6, F[1] + 1.0, -8.6, -8.8, F[1] + 1.25, -7.7, { solid = false })
  B.box("iron", -7.2, F[1], -6.6, -6.4, F[1] + 0.8, -6.0)            -- anvil
  prts.scene.light{ pos = { -9.7, F[1] + 1.8, -7.6 }, color = { 1.3, 0.55, 0.2 }, radius = 9 }
  M.forge_sound = ambience("sounds/forge_loop.wav", -9.7, F[1] + 1.0, -8.1, 1.0, 25)
  prts.scene.zone("keep.forge", { -11, F[1] - 0.1, -9 }, { -4.9, F[2] - M.SLAB, -4.3 })
end

local function quarters()
  local F = M.FLOORS
  for i = 0, 3 do
    local z = -7.5 + i * 3.2
    if z > -3.5 then z = z + 2.5 end
    B.box("oak", -6.8, F[2], z, -4.8, F[2] + 0.5, z + 1.0)           -- bunks
    B.box("planks", -6.8, F[2] + 0.5, z, -4.8, F[2] + 0.6, z + 1.0)
  end
  B.box("oak", 1, F[2], -8.6, 3, F[2] + 0.8, -7.8)                   -- chest
  torch(-1.0, F[2] + 2.3, -8.9, 0, 1)
  torch(4.0, F[2] + 2.3, 8.9, 0, -1)
  prts.scene.zone("keep.floor2", { -11, F[2] - 0.1, -9 }, { 11, F[3] - M.SLAB, 9 })
end

local function war_room()
  local F = M.FLOORS
  local y = F[3]
  B.box("rug", -4, y, -0.5, 4, y + 0.02, 5.5, { solid = false })
  B.box("table", -2.2, y + 0.85, 1.2, 2.2, y + 1.0, 3.8)             -- the war table
  for _, c in ipairs({ { -2.0, 1.4 }, { 1.8, 1.4 }, { -2.0, 3.4 }, { 1.8, 3.4 } }) do
    B.box("oak", c[1], y, c[2], c[1] + 0.2, y + 0.85, c[2] + 0.2)
  end
  prts.scene.illusion{ center = { 0, y + 1.04, 2.5 }, size_x = 4.2, size_z = 2.4, relief = 0.42, levels = 9,
                       area = { x0 = -448, z0 = -256, x1 = 448, z1 = 256 }, color = { 1.0, 0.78, 0.32 } }
  B.box("flame", -0.06, y + 1.0, 2.44, 0.06, y + 1.12, 2.56, { solid = false })  -- the keep's own sigil
  for i, x in ipairs({ -6.5, 4.5 }) do                               -- clerks' desks
    B.box("oak", x, y, -7.5, x + 2, y + 0.8, -6.6)
    B.box("planks", x, y + 0.8, -7.5, x + 2, y + 0.86, -6.6)
    prts.scene.light{ pos = { x + 1, y + 1.3, -7.0 }, color = { 0.9, 0.7, 0.4 }, radius = 5 + i }
  end
  B.box("banner", -5.0, y + 1.0, -8.98, -3.0, y + 3.6, -8.92, { solid = false })
  B.box("banner", -1.0, y + 1.0, -8.98, 1.0, y + 3.6, -8.92, { solid = false })
  B.box("banner", 3.0, y + 1.0, -8.98, 5.0, y + 3.6, -8.92, { solid = false })
  torch(-7.5, y + 2.4, 8.9, 0, -1)
  torch(7.5, y + 2.4, 8.9, 0, -1)
  torch(10.9, y + 2.4, 3.0, -1, 0)
  torch(-6.0, y + 2.4, -8.9, 0, 1)
  prts.scene.light{ pos = { 0, y + 2.4, 2.5 }, color = { 0.9, 0.7, 0.4 }, radius = 9 }
  prts.scene.zone("keep.floor3.war_room", { -11, y - 0.1, -9 }, { 11, M.ROOF - M.SLAB, 9 })
end

local function roof()
  local HX, HZ = M.HX, M.HZ
  B.box("stone_floor", -HX, M.ROOF - M.SLAB, -HZ, HX, M.ROOF, HZ)
  local y0, y1 = M.ROOF, M.ROOF + 1.2
  for x = -HX, HX - 1.2, 2.0 do
    B.box("stone", x, y0, HZ - 0.6, x + 1.2, y1, HZ)
    B.box("stone", x, y0, -HZ, x + 1.2, y1, -HZ + 0.6)
  end
  for z = -HZ + 0.6, HZ - 1.8, 2.0 do
    B.box("stone", HX - 0.6, y0, z, HX, y1, z + 1.2)
    B.box("stone", -HX, y0, z, -HX + 0.6, y1, z + 1.2)
  end
  prts.scene.prism{ base = { 9.6, M.ROOF, -7.6 }, radius = 2.2, top_radius = 2.2, height = 4.0, sides = 10, material = "stone", solid = true }
  prts.scene.prism{ base = { 9.6, M.ROOF + 4.0, -7.6 }, radius = 2.8, top_radius = 0, height = 3.6, sides = 10, material = "slate" }
  B.box("oak", 9.55, M.ROOF + 7.4, -7.65, 9.65, M.ROOF + 10, -7.55, { solid = false })
  B.box("banner", 9.65, M.ROOF + 8.6, -7.62, 11.6, M.ROOF + 9.8, -7.58, { solid = false })
end

local function cottage(x, z, w, d)
  local y = prts.scene.height(x, z)
  B.box("plaster", x - w / 2, y - 0.5, z - d / 2, x + w / 2, y + 2.8, z + d / 2)
  prts.scene.prism{ base = { x, y + 2.8, z }, radius = math.max(w, d) * 0.75, top_radius = 0, height = 2.4, sides = 4, material = "slate" }
  B.box("oak", x - 0.5, y, z + d / 2, x + 0.5, y + 2.0, z + d / 2 + 0.05, { solid = false })
end

local function watchtower(x, z)
  local y = prts.scene.height(x, z)
  B.box("stone", x - 2, y - 0.5, z - 2, x + 2, y + 9, z + 2)
  B.box("planks", x - 2.6, y + 9, z - 2.6, x + 2.6, y + 9.3, z + 2.6)
  for _, c in ipairs({ { -2.6, -2.6 }, { 2.4, -2.6 }, { -2.6, 2.4 }, { 2.4, 2.4 } }) do
    B.box("oak", x + c[1], y + 9.3, z + c[2], x + c[1] + 0.2, y + 11.5, z + c[2] + 0.2)
  end
  prts.scene.prism{ base = { x, y + 11.5, z }, radius = 3.8, top_radius = 0, height = 2.6, sides = 4, material = "slate" }
  prts.scene.light{ pos = { x, y + 10.4, z }, color = { 1.0, 0.6, 0.3 }, radius = 10 }
  B.box("flame", x - 0.15, y + 9.3, z - 0.15, x + 0.15, y + 9.8, z + 0.15, { solid = false })
end

local function countryside()
  -- the road to the keep's door
  for i = 0, 15 do
    local z0 = 10 + i * 6
    local y = prts.scene.height(0, z0 + 3)
    B.box("dirt", -1.6, y - 0.3, z0, 1.6, y + 0.04, z0 + 6, { solid = false })
  end
  watchtower(30, 70)
  cottage(-26, 52, 6, 5)
  cottage(-36, 64, 5, 5)
  cottage(22, 40, 5, 6)
  local rand = B.rng(1407)
  local placed = 0
  for _ = 1, 400 do
    local a, r = rand(0, 2 * math.pi), rand(75, 480)
    local x, z = math.cos(a) * r, math.sin(a) * r
    if math.abs(x) > 6 or z < 0 then
      local y = prts.scene.height(x, z)
      local s = rand(0.8, 1.4)
      prts.scene.prism{ base = { x, y - 0.2, z }, radius = 0.35 * s, top_radius = 0.25 * s, height = 3 * s, sides = 5, material = "bark", solid = true }
      prts.scene.prism{ base = { x, y + 2.2 * s, z }, radius = 2.4 * s, top_radius = 0, height = 4.5 * s, sides = 6, material = "pine" }
      prts.scene.prism{ base = { x, y + 4.6 * s, z }, radius = 1.7 * s, top_radius = 0, height = 3.4 * s, sides = 6, material = "pine" }
      placed = placed + 1
    end
  end
  for _ = 1, 60 do
    local x, z = rand(-400, 400), rand(-400, 400)
    if x * x + z * z > 90 * 90 then
      local y, s = prts.scene.height(x, z), rand(0.6, 2.2)
      B.box("rock", x - s, y - 0.4, z - s * 0.8, x + s, y + s * 0.7, z + s * 0.8)
    end
  end
  return placed
end

function M.build()
  materials()
  prts.scene.terrain{ size = 1024, spacing = 4, seed = 7, height = 26, octaves = 4, material = "grass",
                      flats = { { x = 0, z = 15, radius = 48, height = 0, blend = 70 } } }
  prts.scene.env{
    sun_dir = { -0.75, 0.22, 0.25 }, sun_color = { 1.05, 0.66, 0.42 }, ambient = { 0.20, 0.22, 0.32 },
    fog_color = { 0.56, 0.44, 0.46 }, fog_near = 90, fog_far = 780,
    sky_horizon = { 0.93, 0.56, 0.42 }, sky_zenith = { 0.13, 0.17, 0.38 }, stars = 0.7, moons = true,
  }
  B.box("stone_floor", -M.HX, -0.4, -M.HZ, M.HX, M.FLOORS[1], -1.9)   -- ground floor (with the lift pit)
  B.box("stone_floor", -M.HX, -0.4, 1.9, M.HX, M.FLOORS[1], M.HZ)
  B.box("stone_floor", -7.6, -0.4, -1.9, M.HX, M.FLOORS[1], 1.9)
  outer_walls()
  stairwell()
  slab(M.FLOORS[2])
  slab(M.FLOORS[3])
  M.lift = lift_shaft()
  great_hall()
  forge_room()
  quarters()
  war_room()
  roof()
  M.door = Door.make{ name = "keep.door", hinge = { -1.1, M.FLOORS[1], M.HZ - 0.5 }, width = 2.2, height = 3.2,
                      thickness = 0.16, material = "oak" }
  M.trees = countryside()

  local F = M.FLOORS
  prts.scene.marker("start", { 0, 0, 42 }, 0)
  prts.scene.marker("keep_door", { 0, 0, 11.8 }, 0)
  prts.scene.marker("keep_hall_lift", { -6.4, F[1], 1.65 }, math.pi / 2)
  prts.scene.marker("stair_base", { 9.8, F[1], -1.2 }, 0)
  prts.scene.marker("hall_view", { 8.5, F[1], 7.5 }, math.pi / 4)
  prts.scene.marker("war_room_view", { 0.2, F[3], 7.4 }, 0)
  prts.scene.marker("forge_door", { -8.4, F[1], -2.8 }, 0)
  return M
end

return M
