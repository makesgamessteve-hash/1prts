-- Ashford Vale: the skirmish map of M2. A green valley 1 km long between wooded hills: the
-- Crown's holdfast in the west, a rival lord's in the east, gold in the hills near each and in
-- the middle by the old ford, forests along both sides. Morning light. MIT License.
local S = require("rts.state")
local E = require("rts.economy")
local B = require("lib.build")

local M = {}
M.ID = "ashford_vale"
M.WEST = { x = -380, z = 0 }  -- the Crown (the player in a skirmish)
M.EAST = { x = 380, z = 0 }   -- the rival lord
M.HALF = { x = 500, z = 300 } -- the playing area
M.music = "music/proving_ground.ogg"

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.82, 0.98, 0.72 }, uv_scale = 4 }
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
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  -- the commander's war table and held board show in-world UI surfaces (rts/tools.lua)
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
end

-- A wood: trees in a band around (x, z), from a fixed seed.
local function forest(rand, x, z, w, d, n)
  for _ = 1, n do
    local tx, tz = x + rand(-w, w), z + rand(-d, d)
    E.add_tree(tx, tz, rand(0.8, 1.3))
  end
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 1280, spacing = 4, seed = 31, height = 22, octaves = 4, material = "grass",
                      flats = { { x = M.WEST.x, z = M.WEST.z, radius = 90, height = 2, blend = 60 },
                                { x = M.EAST.x, z = M.EAST.z, radius = 90, height = 2, blend = 60 },
                                { x = 0, z = 0, radius = 260, height = 0, blend = 140 } } }
  prts.scene.env{
    sun_dir = { 0.45, 0.6, -0.35 }, sun_color = { 1.0, 0.92, 0.78 }, ambient = { 0.34, 0.36, 0.42 },
    fog_color = { 0.70, 0.76, 0.84 }, fog_near = 160, fog_far = 900,
    sky_horizon = { 0.78, 0.84, 0.92 }, sky_zenith = { 0.36, 0.52, 0.80 }, stars = 0, moons = false,
  }
  S.match() -- the resources belong to the match registry
  local rand = B.rng(2024)
  -- gold: two mines near each holdfast, two by the ford
  for _, base in ipairs({ M.WEST, M.EAST }) do
    local s = base.x < 0 and -1 or 1
    E.add_mine(base.x - s * 30, base.z - 55)
    E.add_mine(base.x + s * 12, base.z + 58)
  end
  E.add_mine(-20, -150)
  E.add_mine(25, 150)
  -- forests along the valley sides and behind each holdfast
  forest(rand, M.WEST.x - 90, 0, 25, 120, 60)
  forest(rand, M.EAST.x + 90, 0, 25, 120, 60)
  forest(rand, -200, -220, 120, 30, 50)
  forest(rand, 200, 220, 120, 30, 50)
  forest(rand, -200, 220, 120, 30, 40)
  forest(rand, 200, -220, 120, 30, 40)
  -- the old ford: a stony track across the middle
  for i = -8, 8 do
    local x = i * 6
    local y = prts.scene.height(x, 0)
    B.box("dirt", x - 3, y - 0.3, -4, x + 3, y + 0.04, 4, { solid = false })
  end
  -- the commander starts in the war room of the west holdfast, at the war table's south edge
  prts.scene.marker("start", { M.WEST.x, prts.scene.height(M.WEST.x, M.WEST.z) + 0.06, M.WEST.z + 2.6 }, 0)
  prts.scene.marker("gate", { M.WEST.x, prts.scene.height(M.WEST.x, M.WEST.z + 16), M.WEST.z + 16 }, 0)
  return true
end

-- The default skirmish: the player commands the west, an AI the east.
M.SIDES = {
  { id = 1, name = "Crown of Esterholt", color = "crown", ai = false, x = M.WEST.x, z = M.WEST.z },
  { id = 2, name = "Lord Varric's host", color = "red", ai = true, x = M.EAST.x, z = M.EAST.z },
}

-- After the map is finished: the navigation grid, then the match.
function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  if not (opts and opts.no_match) then
    require("rts.match").start{ sides = M.SIDES, serfs = 4 }
  end
  return true
end

prts.events.on("session_started", "maps.ashford_vale", M.on_started)
return M
