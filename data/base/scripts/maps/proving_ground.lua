-- The Proving Ground: Harrowmark, a great concentric castle (an outer and an inner curtain with
-- twenty towers, two gatehouses and the Raven Tower) around a three-storey keep (switchback and
-- spiral stairs, a hoist lift and a war room with its war table) in open country at dusk.
-- MIT License.
local B = require("lib.build")
local Door = require("props.door")
local Lift = require("props.lift")
local NPC = require("props.npc")
local Notice = require("props.notice_board")
local Castle = require("lib.castle")

local M = {}

-- Keep dimensions (meters). The keep is centred on the origin, its door faces +z (south).
M.HX, M.HZ, M.T = 12, 10, 1.0
M.FLOORS = { 0.2, 4.7, 9.2 }  -- walking height of each storey
M.SLAB = 0.3
M.ROOF = 13.7
M.LIFT = { x = -9.3, z = 0, gate = -7.4 }
-- The spiral stair from the war room floor to the roof, under the roof turret's hood.
M.SPIRAL = { x = 9.6, z = -7.35, radius = 0.9, start = -math.pi / 3, steps = 18 }
M.HATCH = { x0 = 8.2, x1 = 11.0, z0 = -8.8, z1 = -5.9 }  -- the opening in the roof above it
M.music = "music/proving_ground.ogg"

-- Harrowmark, the castle around the Great Keep. Coordinates are the inner faces of the walls.
M.INNER = { x0 = -44, x1 = 44, z0 = -48, z1 = 40, thick = 3, height = 15 }   -- the Inner Ward
M.OUTER = { x0 = -80, x1 = 80, z0 = -84, z1 = 100, thick = 2.5, height = 10 } -- the Outer Ward
M.RAVEN = { x = 0, z = -53, radius = 9.5, wall = 2.5, height = 36 }           -- the great tower
M.LION_GATE = { x = 0, half = 9 }                                             -- inner gatehouse
M.KINGS_GATE = { x = 0, half = 10 }                                           -- outer gatehouse

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
  m{ id = "war_table_view", surface = "war_table", magic = true, emissive = true }
end

M.INSIDE = { "keep.hall", "keep.forge", "keep.stairs", "keep.lift", "keep.floor2", "keep.floor3" }

-- Rooms of the keep. Geometry is tagged with the cells it can be seen from, and the renderer
-- only draws cells visible through open portals (doors, windows, stair and lift openings).
local function define_cells()
  local F, c = M.FLOORS, prts.scene.cell
  local top = M.ROOF - M.SLAB
  c{ name = "keep.hall", min = { -11, -0.5, -9 }, max = { 11, F[2] - M.SLAB, 9 } }
  c{ name = "keep.forge", min = { -11, -0.5, -9 }, max = { -4.9, F[2] - M.SLAB, -3.9 } }
  c{ name = "keep.stairs", min = { 6, -0.5, -9 }, max = { 11, top, -1 } }
  c{ name = "keep.lift", min = { -11, -0.7, -2.3 }, max = { -7.6, top, 2.3 } }
  c{ name = "keep.floor2", min = { -11, F[2] - M.SLAB, -9 }, max = { 11, F[3] - M.SLAB, 9 } }
  c{ name = "keep.floor3", min = { -11, F[3] - M.SLAB, -9 }, max = { 11, top, 9 } }
end

local function define_portals()
  local F, p = M.FLOORS, prts.scene.portal
  p{ name = "keep.door", a = "outside", b = "keep.hall", min = { -1.1, F[1], M.HZ - M.T }, max = { 1.1, F[1] + 3.2, M.HZ } }
  p{ name = "keep.forge.door", a = "keep.hall", b = "keep.forge", min = { -9.0, F[1], -4.3 }, max = { -7.8, F[1] + 2.4, -3.9 } }
  local h = M.HATCH
  p{ name = "keep.roof.hatch", a = "keep.stairs", b = "outside", min = { h.x0, M.ROOF - M.SLAB - 0.05, h.z0 },
     max = { h.x1, M.ROOF + 0.05, h.z1 } }
  for f, cell in ipairs({ "keep.hall", "keep.floor2", "keep.floor3" }) do
    p{ name = "keep.stairs." .. f, a = "keep.stairs", b = cell, min = { 6, F[f], -1.05 }, max = { 11, F[f] + 3.9, -0.95 } }
    p{ name = "keep.lift.gate" .. (f - 1), a = "keep.lift", b = cell, min = { -7.6, F[f], -1.2 }, max = { -7.2, F[f] + 2.8, 1.2 } }
  end
end

-- A window is a hole in an outer wall and a portal between its room and the outside.
local function window_portal(min, max, inward_x, inward_z)
  local cx, cy, cz = (min[1] + max[1]) / 2, (min[2] + max[2]) / 2, (min[3] + max[3]) / 2
  local room = prts.scene.cell_at(cx + inward_x, cy, cz + inward_z)
  prts.scene.portal{ name = "window." .. room .. "." .. cx .. "." .. cz, a = room, b = "outside", min = min, max = max }
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
-- Places a model (glTF or .vox); opts: yaw, scale, solid, name (marker prefix).
local function model(path, x, y, z, opts)
  opts = opts or {}
  return prts.scene.model{ path = path, pos = { x, y, z }, yaw = opts.yaw or 0, scale = opts.scale or 0,
                           solid = opts.solid or false, name = opts.name }
end

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
  local function portals(list, axis, t0, t1, inward)
    for _, w in ipairs(list) do
      if axis == "x" then
        window_portal({ w.from, w.bottom, t0 }, { w.to, w.top, t1 }, 0, inward)
      else
        window_portal({ t0, w.bottom, w.from }, { t1, w.top, w.to }, inward, 0)
      end
    end
  end
  -- south wall with the main door
  local south = windows({ -7, -3.5, 3.5, 7 })
  portals(south, "x", HZ - T, HZ, -1.5)
  south[#south + 1] = { from = -1.1, to = 1.1, bottom = 0.2, top = 3.4 }
  B.wall("stone", "x", -HX, HX, HZ - T, HZ, 0, M.ROOF, south)
  local north, east, west = windows({ -7, -2, 2, 7 }), windows({ 2, 6 }), windows({ -6, 6 })
  portals(north, "x", -HZ, -HZ + T, 1.5)
  portals(east, "z", HX - T, HX, -1.5)
  portals(west, "z", -HX, -HX + T, 1.5)
  B.wall("stone", "x", -HX, HX, -HZ, -HZ + T, 0, M.ROOF, north)
  B.wall("stone", "z", -HZ + T, HZ - T, HX - T, HX, 0, M.ROOF, east)
  B.wall("stone", "z", -HZ + T, HZ - T, -HX, -HX + T, 0, M.ROOF, west)
end

local function stairwell()
  local F = M.FLOORS
  -- inner wall between the great hall and the stairwell (seen from both sides)
  prts.scene.cells{ "keep.stairs", "keep.hall", "keep.floor2", "keep.floor3" }
  B.box("plaster", 5.4, 0, -9, 6, M.ROOF - M.SLAB, -1)
  prts.scene.cells{ "keep.stairs" }
  for f = 1, 3 do torch(10.9, F[f] + 2.4, -2.0, -1, 0) end
  torch(8.0, F[1] + 4.6, -8.9, 0, 1)
  torch(8.0, F[2] + 4.6, -8.9, 0, 1)
  B.box("plaster", 8.4, 0, -5.7, 8.6, M.ROOF - M.SLAB, -3)  -- the wall between flights
  for f = 1, 2 do
    local base = F[f]
    local mid = B.stairs("stone_floor", { axis = "z", sign = -1, start = -3, w0 = 8.6, w1 = 11, steps = 9,
                                          rise = 0.25, run = 0.3, base = base, solid = f == 1 })
    B.box("stone_floor", 6, mid - 0.3, -9, 11, mid, -5.7)  -- landing
    B.stairs("stone_floor", { axis = "z", sign = 1, start = -5.7, w0 = 6, w1 = 8.4, steps = 9,
                              rise = 0.25, run = 0.3, base = mid })
  end
  -- war room level: a gallery above the lower flight and a floor above the landing, where the
  -- spiral stair climbs through the roof hatch
  prts.scene.cells{ "keep.stairs", "keep.floor3" }
  local F3 = F[3]
  B.box("planks", 8.6, F3 - M.SLAB, -5.7, 11, F3, -3)
  B.box("planks", 6, F3 - M.SLAB, -9, 11, F3, -5.7)
  prts.scene.cells{ "keep.stairs", "keep.floor3", "outside" }
  local sp = M.SPIRAL
  M.spiral_top = B.spiral("stone_floor", { center = { sp.x, sp.z }, base = F3, steps = sp.steps, radius = sp.radius,
                                           start = sp.start, post = "oak", headroom = 1.0 })
  prts.scene.light{ pos = { 10.4, F3 + 2.6, -5.0 }, color = { 1.2, 0.7, 0.35 }, radius = 8 }
end

local function lift_shaft()
  local F, L = M.FLOORS, M.LIFT
  prts.scene.cells{ "keep.lift", "keep.hall", "keep.floor2", "keep.floor3" }
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
  prts.scene.cells{ "keep.hall" }
  for _, x in ipairs({ -4, 2 }) do
    for _, z in ipairs({ -4, 1, 6 }) do B.box("stone", x - 0.35, F[1], z - 0.35, x + 0.35, F[2] - M.SLAB, z + 0.35) end
  end
  B.box("table", -3, F[1] + 0.75, 2.6, 1, F[1] + 0.85, 4.0)          -- long table
  B.box("oak", -2.9, F[1], 2.7, -2.7, F[1] + 0.75, 3.9)
  B.box("oak", 0.7, F[1], 2.7, 0.9, F[1] + 0.75, 3.9)
  for _, z in ipairs({ 2.1, 4.5 }) do                                 -- benches
    for _, x in ipairs({ -2.0, 0.0 }) do model("models/bench.glb", x, F[1], z, { solid = true }) end
  end
  -- the castellan's orders on the stairwell wall (an in-world UI surface)
  local orders = require("story.harrowmark").NOTICES
  M.notices = Notice.make{ id = "hall.notices", x = 5.38, y = F[1] + 1.75, z = -4.6, facing = "-x", width = 1.6,
                           height = 1.0, title = orders.title, lines = orders.lines }
  for i, x in ipairs({ -3.2, 2.2 }) do                                -- braziers by the hearth
    local name = "hall.brazier" .. i
    model("models/brazier.glb", x, F[1], -1.0, { solid = true, name = name })
    local lx, ly, lz = prts.scene.marker_pos(name .. ".light")
    prts.scene.light{ pos = { lx, ly, lz }, color = { 1.4, 0.72, 0.32 }, radius = 8 }
  end
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
  prts.scene.cells{ "keep.forge", "keep.hall" }
  B.door_frame("plaster", "x", -11, -4.9, -4.3, -3.9, F[1] - 0.2, F[2] - M.SLAB,
               { from = -9.0, to = -7.8, height = 2.4 + 0.2 })
  B.box("plaster", -5.3, F[1] - 0.2, -9, -4.9, F[2] - M.SLAB, -4.3)
  Door.make{ name = "keep.forge.door", hinge = { -9.0, F[1], -4.1 }, width = 1.2, height = 2.4, material = "oak",
             portal = "keep.forge.door" }
  prts.scene.cells{ "keep.forge" }
  B.box("stone", -10.9, F[1], -8.9, -8.5, F[1] + 1.0, -7.4)          -- hearth
  B.box("flame", -10.6, F[1] + 1.0, -8.6, -8.8, F[1] + 1.25, -7.7, { solid = false })
  model("models/anvil.vox", -6.8, F[1], -6.3, { yaw = 0.3, scale = 0.05, solid = true })
  model("models/crate.vox", -10.3, F[1], -5.0, { scale = 0.07, solid = true })
  model("models/crate.vox", -10.3, F[1] + 0.84, -5.0, { yaw = 0.4, scale = 0.07, solid = true })
  model("models/crate.vox", -10.2, F[1], -6.0, { yaw = 0.1, scale = 0.07, solid = true })
  prts.scene.light{ pos = { -9.7, F[1] + 1.8, -7.6 }, color = { 1.3, 0.55, 0.2 }, radius = 9 }
  M.forge_sound = ambience("sounds/forge_loop.wav", -9.7, F[1] + 1.0, -8.1, 1.0, 25)
  prts.scene.zone("keep.forge", { -11, F[1] - 0.1, -9 }, { -4.9, F[2] - M.SLAB, -4.3 })
end

local function quarters()
  local F = M.FLOORS
  prts.scene.cells{ "keep.floor2" }
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
  prts.scene.cells{ "keep.floor3" }
  local y = F[3]
  B.box("rug", -4, y, -0.5, 4, y + 0.02, 5.5, { solid = false })
  B.box("table", -2.2, y + 0.85, 1.2, 2.2, y + 1.0, 3.8)             -- the war table
  for _, c in ipairs({ { -2.0, 1.4 }, { 1.8, 1.4 }, { -2.0, 3.4 }, { 1.8, 3.4 } }) do
    B.box("oak", c[1], y, c[2], c[1] + 0.2, y + 0.85, c[2] + 0.2)
  end
  -- a live picture of the valley from high above, conjured onto the table top
  prts.scene.surface{ name = "war_table", pos = { 0, 260, 15 }, yaw = 0, pitch = -math.pi / 2, fov = 60, size = { 160, 91 } }
  B.box("war_table_view", -2.1, y + 1.0, 1.3, 2.1, y + 1.012, 3.7, { solid = false, fit = true })
  prts.scene.illusion{ center = { 0, y + 1.04, 2.5 }, size_x = 4.2, size_z = 2.4, relief = 0.42, levels = 9,
                       area = { x0 = -262, z0 = -135, x1 = 262, z1 = 165 }, color = { 1.0, 0.78, 0.32 } }
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
  prts.scene.cells{ "outside", "keep.floor3", "keep.stairs" }
  local h, y0r = M.HATCH, M.ROOF - M.SLAB
  B.box("stone_floor", -HX, y0r, -HZ, h.x0, M.ROOF, HZ)      -- the roof around the hatch
  B.box("stone_floor", h.x1, y0r, -HZ, HX, M.ROOF, HZ)
  B.box("stone_floor", h.x0, y0r, -HZ, h.x1, M.ROOF, h.z0)
  B.box("stone_floor", h.x0, y0r, h.z1, h.x1, M.ROOF, HZ)
  prts.scene.cells{ "outside" }
  local y0, y1 = M.ROOF, M.ROOF + 1.2
  for x = -HX, HX - 1.2, 2.0 do
    B.box("stone", x, y0, HZ - 0.6, x + 1.2, y1, HZ)
    B.box("stone", x, y0, -HZ, x + 1.2, y1, -HZ + 0.6)
  end
  for z = -HZ + 0.6, HZ - 1.8, 2.0 do
    B.box("stone", HX - 0.6, y0, z, HX, y1, z + 1.2)
    B.box("stone", -HX, y0, z, -HX + 0.6, y1, z + 1.2)
  end
  -- the turret over the spiral stair: an open hood on four posts
  for _, c in ipairs({ { h.x0, h.z0 }, { h.x1 - 0.25, h.z0 }, { h.x0, h.z1 - 0.25 }, { h.x1 - 0.25, h.z1 - 0.25 } }) do
    B.box("oak", c[1], M.ROOF, c[2], c[1] + 0.25, M.ROOF + 3.0, c[2] + 0.25)
  end
  B.box("oak", h.x0 - 0.2, M.ROOF + 3.0, h.z0 - 0.2, h.x1 + 0.2, M.ROOF + 3.3, h.z1 + 0.2)
  prts.scene.prism{ base = { 9.6, M.ROOF + 3.3, -7.35 }, radius = 2.6, top_radius = 0, height = 3.0, sides = 10, material = "slate" }
  B.box("oak", 9.55, M.ROOF + 5.8, -7.4, 9.65, M.ROOF + 8.4, -7.3, { solid = false })
  B.box("banner", 9.65, M.ROOF + 7.0, -7.37, 11.6, M.ROOF + 8.2, -7.33, { solid = false })
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

-- Banners of the Crown on a gatehouse's outer face, and lights in its passage.
local function dress_gate(gate, outer_z, outward, tower_radius, tower_height, passage_height)
  -- a banner down the front of each drum tower
  for _, sx in ipairs({ -1, 1 }) do
    local tx = gate.x + sx * (gate.half - tower_radius * 0.6)
    local z = outer_z + outward * (tower_radius + 0.08)
    B.box("banner", tx - 0.9, tower_height - 8, math.min(z, z + outward * 0.06), tx + 0.9, tower_height - 1.5,
          math.max(z, z + outward * 0.06), { solid = false })
  end
  prts.scene.light{ pos = { gate.x, passage_height - 1.0, outer_z - outward * 2 }, color = { 1.3, 0.75, 0.4 }, radius = 10 }
  prts.scene.light{ pos = { gate.x, passage_height - 1.0, outer_z + outward * 3 }, color = { 1.3, 0.75, 0.4 }, radius = 9 }
end

-- The garrison: guards at the gates, at the Raven Tower and walking the inner wall-walk. Who
-- they are and what they say is in story/harrowmark.lua (PEOPLE, by the name after "npc.").
local function garrison()
  local I, O, R = M.INNER, M.OUTER, M.RAVEN
  local guard = "sprites/crown_guard.lua"
  local who = require("story.harrowmark").PEOPLE
  local south = O.z1 + O.thick + 4  -- the King's Gate's outer face
  -- the gate guards stand in the gate passages' mouths, between the drum towers: inside a tower's
  -- footprint its wall boxes would hide them from the use ray
  local list = {
    NPC.make{ name = "npc.kings_gate_left", sprite = guard, pos = { -1.6, 0, south + 1.2 }, yaw = math.pi,
              person = who.kings_gate_left },
    NPC.make{ name = "npc.kings_gate_right", sprite = guard, pos = { 1.6, 0, south + 1.2 }, yaw = math.pi,
              person = who.kings_gate_right },
    NPC.make{ name = "npc.lion_gate", sprite = guard, pos = { 1.5, 0, I.z1 + I.thick + 4.2 }, yaw = math.pi,
              person = who.lion_gate },
    NPC.make{ name = "npc.raven_door", sprite = guard, pos = { R.x + 2.2, 0, R.z + R.radius + 1.2 }, yaw = math.pi,
              person = who.raven_door },
    NPC.make{ name = "npc.wall_patrol", sprite = guard, pos = { -34, I.height, I.z1 + 1.2 }, speed = 1.1,
              patrol = { { -12, I.height, I.z1 + 1.2 }, { -34, I.height, I.z1 + 1.2 } }, person = who.wall_patrol },
    NPC.make{ name = "npc.outer_patrol", sprite = guard, pos = { -60, 0, 80 }, speed = 1.2,
              patrol = { { 60, 0, 80 }, { 60, 0, -70 }, { -60, 0, -70 }, { -60, 0, 80 } }, person = who.outer_patrol },
  }
  return #list
end

-- The Inner Ward: a 15 m curtain with corner and side towers, the Raven Tower on the north
-- side (with a spiral stair to its top) and the Lion Gate on the south side.
local function inner_ward()
  local I, R = M.INNER, M.RAVEN
  local t, h = I.thick, I.height
  local cx0, cx1, cz0, cz1 = I.x0 - t / 2, I.x1 + t / 2, I.z0 - t / 2, I.z1 + t / 2
  local corner = { radius = 6.5, height = 22 }
  local towers = {
    { x = cx0, z = cz0, radius = corner.radius, height = corner.height },
    { x = cx1, z = cz0, radius = corner.radius, height = corner.height },
    { x = cx0, z = cz1, radius = corner.radius, height = corner.height },
    { x = cx1, z = cz1, radius = corner.radius, height = corner.height },
    { x = cx0, z = -4, radius = 5.5, height = 20 },
    { x = cx1, z = -4, radius = 5.5, height = 20 },
    { x = R.x, z = R.z, radius = R.radius, wall = R.wall, height = R.height,
      stair = { radius = 2.6, size = 1.3, rise = 0.3, start = math.pi / 2, post = "oak" } },
  }
  Castle.curtain_ring{ x0 = I.x0, x1 = I.x1, z0 = I.z0, z1 = I.z1, thick = t, height = h, material = "stone",
                       floor_material = "stone_floor", towers = towers,
                       gates = { { side = "south", a0 = M.LION_GATE.x - M.LION_GATE.half, a1 = M.LION_GATE.x + M.LION_GATE.half } } }
  M.lion_gate = Castle.gatehouse{ name = "harrowmark.lion_gate", x = M.LION_GATE.x, z0 = I.z1 - 3, z1 = I.z1 + t + 3,
                                  half = M.LION_GATE.half, height = h, outward = 1, passage = { half = 2.5, height = 5.5 },
                                  tower = { radius = 4.5, height = 21 }, material = "stone", floor_material = "stone_floor" }
  -- stairs up to the wall-walk along the inner face of the south and north walls
  dress_gate(M.LION_GATE, I.z1 + t + 3, 1, 4.5, 21, 5.5)
  Castle.wall_stair("stone", { axis = "x", sign = 1, start = -38, w0 = I.z1 - 1.8, w1 = I.z1, base = 0, top = h })
  Castle.wall_stair("stone", { axis = "x", sign = -1, start = 38, w0 = I.z0, w1 = I.z0 + 1.8, base = 0, top = h })
  -- the chapel, the kitchens and the well
  Castle.hall{ x0 = 18, z0 = -40, x1 = 30, z1 = -20, height = 7, ridge = 4, material = "plaster",
               door = { side = "x0", at = -30, width = 2 } }
  Castle.hall{ x0 = -34, z0 = 18, x1 = -18, z1 = 30, height = 5, ridge = 3, material = "plaster",
               door = { side = "x1", at = 24, width = 1.6 } }
  prts.scene.tube{ base = { -22, -1, -26 }, radius = 1.4, inner = 0.9, height = 1.9, sides = 12, material = "stone" }
  B.box("stone", -23.2, -1, -27.2, -20.8, 0.9, -24.8, { visible = false })
  prts.scene.marker("raven_tower_door", { R.x, 0, R.z + R.radius + 1.5 }, 0)
  prts.scene.marker("inner_wall_stair", { -38.5, 0, I.z1 - 0.9 }, -math.pi / 2)
end

-- The Outer Ward: a 10 m curtain with eleven towers and the King's Gate, where the road comes in;
-- stables, barracks and a smithy inside it.
local function outer_ward()
  local O = M.OUTER
  local t, h = O.thick, O.height
  local cx0, cx1, cz0, cz1 = O.x0 - t / 2, O.x1 + t / 2, O.z0 - t / 2, O.z1 + t / 2
  local towers = {}
  local function tower(x, z) towers[#towers + 1] = { x = x, z = z, radius = 5, wall = 1.3, height = 14 } end
  tower(cx0, cz0) tower(cx1, cz0) tower(cx0, cz1) tower(cx1, cz1)
  for _, x in ipairs({ -40, 0, 40 }) do tower(x, cz0) end
  for _, x in ipairs({ -45, 45 }) do tower(x, cz1) end
  for _, z in ipairs({ -39, 8, 55 }) do tower(cx0, z) tower(cx1, z) end
  Castle.curtain_ring{ x0 = O.x0, x1 = O.x1, z0 = O.z0, z1 = O.z1, thick = t, height = h, material = "stone",
                       floor_material = "stone_floor", towers = towers,
                       gates = { { side = "south", a0 = M.KINGS_GATE.x - M.KINGS_GATE.half, a1 = M.KINGS_GATE.x + M.KINGS_GATE.half } } }
  M.kings_gate = Castle.gatehouse{ name = "harrowmark.kings_gate", x = M.KINGS_GATE.x, z0 = O.z1 - 3, z1 = O.z1 + t + 4,
                                   half = M.KINGS_GATE.half, height = h, outward = 1, passage = { half = 2.75, height = 6 },
                                   tower = { radius = 5, height = 16 }, material = "stone", floor_material = "stone_floor" }
  dress_gate(M.KINGS_GATE, O.z1 + t + 4, 1, 5, 16, 6)
  Castle.wall_stair("stone", { axis = "x", sign = 1, start = 18, w0 = O.z1 - 1.8, w1 = O.z1, base = 0, top = h })
  Castle.wall_stair("stone", { axis = "x", sign = -1, start = -18, w0 = O.z0, w1 = O.z0 + 1.8, base = 0, top = h })
  Castle.hall{ x0 = -76, z0 = -30, x1 = -64, z1 = 0, height = 4.5, ridge = 3, material = "planks",
               roof_material = "slate", door = { side = "x1", at = -15, width = 3 } }       -- stables
  Castle.hall{ x0 = 62, z0 = -10, x1 = 76, z1 = 20, height = 6, ridge = 3.5, material = "plaster",
               door = { side = "x0", at = 5, width = 1.8 } }                                -- barracks
  Castle.hall{ x0 = -60, z0 = 62, x1 = -48, z1 = 72, height = 4.5, ridge = 2.5, material = "stone",
               door = { side = "x1", at = 67, width = 2.2 } }                               -- smithy
  prts.scene.marker("outer_ward", { 0, 0, 72 }, 0)
  prts.scene.marker("outer_wall_stair", { 17.5, 0, O.z1 - 0.9 }, -math.pi / 2)
end

local function countryside()
  prts.scene.cells{ "outside" }
  B.wedge("stone_floor", -1.6, 0, M.HZ, 1.6, M.FLOORS[1], M.HZ + 1.4, { axis = "z", sign = -1 }) -- door ramp
  for _, x in ipairs({ -2.6, 2.6 }) do                                -- banners at the door
    model("models/banner_stand.glb", x, prts.scene.height(x, 10.6), 10.6)
  end
  for _, c in ipairs({ { 4.3, 11.2, 0, 0.2 }, { 5.2, 11.0, 0, -0.3 }, { 4.7, 11.1, 0.84, 0.9 } }) do
    model("models/crate.vox", c[1], prts.scene.height(c[1], c[2]) + c[3], c[2], { yaw = c[4], scale = 0.07, solid = true })
  end
  -- the road from the countryside through both gates to the keep's door
  for i = 0, 23 do
    local z0 = 10 + i * 6
    local y = prts.scene.height(0, z0 + 3)
    B.box("dirt", -1.6, y - 0.3, z0, 1.6, y + 0.04, z0 + 6, { solid = false })
  end
  watchtower(30, 70)
  cottage(-26, 52, 6, 5)
  cottage(-36, 64, 5, 5)
  cottage(26, 62, 5, 6)
  local rand = B.rng(1407)
  local placed = 0
  for _ = 1, 400 do
    local a, r = rand(0, 2 * math.pi), rand(150, 480)
    local x, z = math.cos(a) * r, math.sin(a) * r
    if math.abs(x) > 30 or z < 0 then -- the approach to the King's Gate stays open
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
    if x * x + (z - 8) * (z - 8) > 160 * 160 then
      local y, s = prts.scene.height(x, z), rand(0.6, 2.2)
      B.box("rock", x - s, y - 0.4, z - s * 0.8, x + s, y + s * 0.7, z + s * 0.8)
    end
  end
  return placed
end

-- The people of Harrowmark at their posts; who they are and what they say is in
-- story/harrowmark.lua (PEOPLE, by the part of the entity name after "npc.").
local function people()
  local F = M.FLOORS
  local guard, clerk = "sprites/crown_guard.lua", "sprites/crown_clerk.lua"
  local who = require("story.harrowmark").PEOPLE
  local list = {}
  local function person(post, n)
    n.name, n.person = "npc." .. post, who[post]
    list[#list + 1] = NPC.make(n)
  end
  prts.scene.cells{ "outside" }
  person("door_guard", { sprite = guard, pos = { 2.3, 0, 11.4 }, yaw = math.pi })
  person("road_patrol", { sprite = guard, pos = { -2.6, 0, 16 }, patrol = { { -2.6, 0, 46 }, { -2.6, 0, 16 } }, speed = 1.3 })
  prts.scene.cells{ "keep.hall" }
  person("hall_guard", { sprite = guard, pos = { 4.2, F[1], -6.5 }, yaw = -math.pi / 2 })
  person("hall_patrol", { sprite = guard, pos = { -6, F[1], 6 },
                          patrol = { { 3, F[1], 6 }, { 3, F[1], -2 }, { -6, F[1], -2 }, { -6, F[1], 6 } }, speed = 1.2 })
  prts.scene.cells{ "keep.floor3" }
  person("clerk", { sprite = clerk, pos = { -5.5, F[3], -5.9 }, yaw = 0 })
  person("war_room_guard", { sprite = guard, pos = { -2.6, F[3], 6.4 }, yaw = math.pi * 0.85 })
  return #list
end

function M.build()
  materials()
  define_cells()
  define_portals()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 1024, spacing = 4, seed = 7, height = 26, octaves = 4, material = "grass",
                      flats = { { x = 0, z = 8, radius = 140, height = 0, blend = 80 } } }
  prts.scene.env{
    sun_dir = { -0.75, 0.22, 0.25 }, sun_color = { 1.05, 0.66, 0.42 }, ambient = { 0.20, 0.22, 0.32 },
    fog_color = { 0.56, 0.44, 0.46 }, fog_near = 90, fog_far = 780,
    sky_horizon = { 0.93, 0.56, 0.42 }, sky_zenith = { 0.13, 0.17, 0.38 }, stars = 0.7, moons = true,
  }
  prts.scene.cells{ "keep.hall", "keep.forge", "keep.stairs", "keep.lift" }
  B.box("stone_floor", -M.HX, -0.4, -M.HZ, M.HX, M.FLOORS[1], -1.9)   -- ground floor (with the lift pit)
  B.box("stone_floor", -M.HX, -0.4, 1.9, M.HX, M.FLOORS[1], M.HZ)
  B.box("stone_floor", -7.6, -0.4, -1.9, M.HX, M.FLOORS[1], 1.9)
  prts.scene.cells()
  outer_walls()
  stairwell()
  prts.scene.cells{ "keep.hall", "keep.forge", "keep.stairs", "keep.lift", "keep.floor2" }
  slab(M.FLOORS[2])
  prts.scene.cells{ "keep.floor2", "keep.floor3", "keep.stairs", "keep.lift" }
  slab(M.FLOORS[3])
  M.lift = lift_shaft()
  -- a lantern hanging in the lift cage: the light rides up and down with the platform
  prts.scene.cells{ "keep.lift" }
  prts.scene.light{ pos = { 1.2, 2.3, -1.4 }, color = { 1.1, 0.75, 0.4 }, radius = 5, entity = M.lift }
  prts.scene.cells()
  great_hall()
  forge_room()
  quarters()
  war_room()
  roof()
  prts.scene.cells{ "outside", "keep.hall" }
  M.door = Door.make{ name = "keep.door", hinge = { -1.1, M.FLOORS[1], M.HZ - 0.5 }, width = 2.2, height = 3.2,
                      thickness = 0.16, material = "oak", portal = "keep.door" }
  inner_ward()
  outer_ward()
  M.guards = garrison()
  M.trees = countryside()
  M.people = people()
  prts.scene.cells()

  local F = M.FLOORS
  prts.scene.marker("start", { 0, 0, 132 }, 0) -- on the road, before the King's Gate
  prts.scene.marker("keep_door", { 0, 0, 11.8 }, 0)
  prts.scene.marker("keep_hall_lift", { -6.4, F[1], 1.65 }, math.pi / 2)
  prts.scene.marker("stair_base", { 9.8, F[1], -1.2 }, 0)
  prts.scene.marker("hall_view", { 8.5, F[1], 7.5 }, math.pi / 4)
  prts.scene.marker("war_room_view", { 0.2, F[3], 7.4 }, 0)
  prts.scene.marker("forge_door", { -8.4, F[1], -2.8 }, 0)
  local sp = M.SPIRAL
  local a = sp.start - math.pi / 3 -- two steps' turn before the first step, clear of it
  -- facing along the turn: forward (-sin yaw, -cos yaw) = (-sin a, cos a)
  prts.scene.marker("spiral_base", { sp.x + sp.radius * math.cos(a), F[3], sp.z + sp.radius * math.sin(a) },
                    math.atan(math.sin(a), -math.cos(a)))
  return M
end

return M
