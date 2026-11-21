-- Thornmere: the first medium skirmish map (M4), 1.6 km of moor and mere between wooded hills.
-- The Crown holds the west, the Wildborn Clans the east; each has three gold mines and two aether
-- springs near home, and three mines and a spring wait in the middle by the old stones. A stream
-- runs north to south across the middle with fords. MIT License.
local S = require("rts.state")
local E = require("rts.economy")
local B = require("lib.build")

local M = {}
M.ID = "thornmere"
M.WEST = { x = -600, z = 0 }
M.EAST = { x = 600, z = 0 }
M.HALF = { x = 780, z = 460 }
M.music = "music/proving_ground.ogg"

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.80, 0.92, 0.66 }, uv_scale = 4 }
  m{ id = "dirt", texture = "textures/dirt.png", uv_scale = 3 }
  m{ id = "stone", texture = "textures/stone_wall.png", uv_scale = 3 }
  m{ id = "stone_floor", texture = "textures/stone_floor.png", uv_scale = 3 }
  m{ id = "planks", texture = "textures/wood_planks.png", uv_scale = 2 }
  m{ id = "oak", texture = "textures/wood_dark.png", uv_scale = 2.2 }
  m{ id = "plaster", texture = "textures/plaster.png", uv_scale = 3 }
  m{ id = "slate", texture = "textures/roof_slate.png", uv_scale = 2 }
  m{ id = "banner", texture = "textures/banner_crown.png", uv_scale = 1.6 }
  m{ id = "banner_red", texture = "textures/banner_crown.png", tint = { 1.6, 0.35, 0.3 }, uv_scale = 1.6 }
  m{ id = "bark", texture = "textures/bark.png", uv_scale = 1 }
  m{ id = "pine", texture = "textures/pine.png", uv_scale = 2 }
  m{ id = "rock", texture = "textures/rock.png", uv_scale = 2 }
  m{ id = "iron", texture = "textures/iron.png", uv_scale = 1 }
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.35, 0.55, 0.75 }, uv_scale = 4 }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  require("props.horse").materials()
end

local function forest(rand, x, z, w, d, n)
  for _ = 1, n do E.add_tree(x + rand(-w, w), z + rand(-d, d), rand(0.8, 1.3)) end
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 2048, spacing = 4, seed = 91, height = 26, octaves = 4, material = "grass",
                      flats = { { x = M.WEST.x, z = M.WEST.z, radius = 160, height = 2, blend = 80 },
                                { x = M.EAST.x, z = M.EAST.z, radius = 160, height = 2, blend = 80 },
                                { x = 0, z = 0, radius = 360, height = 0, blend = 160 } } }
  prts.scene.env{
    sun_dir = { -0.4, 0.55, -0.4 }, sun_color = { 1.0, 0.88, 0.74 }, ambient = { 0.33, 0.35, 0.40 },
    fog_color = { 0.66, 0.70, 0.74 }, fog_near = 180, fog_far = 1000,
    sky_horizon = { 0.76, 0.80, 0.86 }, sky_zenith = { 0.38, 0.50, 0.74 }, stars = 0, moons = false,
  }
  S.match() -- resources belong to the match registry
  local rand = B.rng(1913)
  for _, base in ipairs({ M.WEST, M.EAST }) do
    local s = base.x < 0 and -1 or 1
    E.add_mine(base.x - s * 30, base.z - 60)
    E.add_mine(base.x + s * 15, base.z + 70)
    E.add_mine(base.x - s * 90, base.z + 20)
    E.add_spring(base.x + s * 60, base.z - 40)
    E.add_spring(base.x + s * 50, base.z + 110)
    forest(rand, base.x - s * 140, 0, 30, 200, 80)
  end
  E.add_mine(-20, -160)
  E.add_mine(30, 170)
  E.add_mine(0, 0)
  E.add_spring(60, -20)
  forest(rand, -300, -330, 200, 40, 70)
  forest(rand, 300, 330, 200, 40, 70)
  forest(rand, -300, 330, 200, 40, 60)
  forest(rand, 300, -330, 200, 40, 60)
  -- the stream and its fords: a shallow band across the middle (walkable: it is low water)
  for i = -40, 40 do
    local z = i * 10
    local x = math.sin(i * 0.25) * 20 + 120
    local y = prts.scene.height(x, z)
    B.box("water", x - 4, y - 0.3, z - 5.5, x + 4, y + 0.03, z + 5.5, { solid = false })
  end
  -- the old stones in the middle
  for i = 0, 7 do
    local a = i / 8 * 2 * math.pi
    local x, z = math.cos(a) * 28, math.sin(a) * 28
    local y = prts.scene.height(x, z)
    B.box("rock", x - 0.8, y - 0.5, z - 0.6, x + 0.8, y + 3.5 + (i % 2), z + 0.6)
  end
  prts.scene.marker("start", { M.WEST.x, prts.scene.height(M.WEST.x, M.WEST.z) + 0.06, M.WEST.z + 2.6 }, 0)
  prts.scene.marker("gate", { M.WEST.x, prts.scene.height(M.WEST.x, M.WEST.z + 18), M.WEST.z + 18 }, 0)
  return true
end

-- The default: the player leads the Crown in the west, an AI the Wildborn in the east.
M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.WEST.x, z = M.WEST.z,
    gold = 400, timber = 300 },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true, x = M.EAST.x, z = M.EAST.z,
    gold = 400, timber = 300 },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -800, -435, 800, 435 } -- what the war table shows
  if not (opts and opts.no_match) then
    require("rts.match").start{ sides = M.SIDES, serfs = 5 }
  end
  return true
end

prts.events.on("session_started", "maps.thornmere", M.on_started)
return M
