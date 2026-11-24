-- Watchfire Hill: a small skirmish from the example mod. A hill with a beacon in the middle of a
-- valley; the Crown in the south, Clan Varrak in the north, a ruin of the Imperium on the hill.
-- A map module builds the map (build) and starts its match when the session starts. MIT License.
local S = require("rts.state")
local B = require("lib.build")

local M = {}
M.ID = "watchfire_hill"
M.SOUTH = { x = 0, z = -300 }
M.NORTH = { x = 0, z = 300 }
M.RUINS = { { x = 60, z = 0 } }
M.music = "music/proving_ground.ogg"

function M.build()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.86, 0.9, 0.68 }, uv_scale = 4 }
  m{ id = "stone", texture = "textures/stone_wall.png", uv_scale = 3 }
  m{ id = "bark", texture = "textures/bark.png", uv_scale = 1 }
  m{ id = "pine", texture = "textures/pine.png", uv_scale = 2 }
  m{ id = "rock", texture = "textures/rock.png", uv_scale = 2 }
  m{ id = "oak", texture = "textures/wood_dark.png", uv_scale = 2.2 }
  m{ id = "dirt", texture = "textures/dirt.png", uv_scale = 3 }
  m{ id = "stone_floor", texture = "textures/stone_floor.png", uv_scale = 3 }
  m{ id = "planks", texture = "textures/wood_planks.png", uv_scale = 2 }
  m{ id = "plaster", texture = "textures/plaster.png", uv_scale = 3 }
  m{ id = "slate", texture = "textures/roof_slate.png", uv_scale = 2 }
  m{ id = "banner", texture = "textures/banner_crown.png", uv_scale = 1.6 }
  m{ id = "banner_red", texture = "textures/banner_crown.png", tint = { 1.6, 0.35, 0.3 }, uv_scale = 1.6 }
  m{ id = "iron", texture = "textures/iron.png", uv_scale = 1 }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 1024, spacing = 4, seed = 99, height = 10, octaves = 4, material = "grass", flats = {
    { x = M.SOUTH.x, z = M.SOUTH.z, radius = 90, height = 2, blend = 60 },
    { x = M.NORTH.x, z = M.NORTH.z, radius = 90, height = 2, blend = 60 },
    { x = 0, z = 0, radius = 40, height = 14, blend = 80 } } } -- the hill
  prts.scene.env{ sun_dir = { 0.5, 0.4, -0.3 }, sun_color = { 1.0, 0.8, 0.6 }, ambient = { 0.34, 0.33, 0.36 },
                  fog_color = { 0.7, 0.66, 0.62 }, fog_near = 150, fog_far = 900, sky_horizon = { 0.9, 0.72, 0.56 },
                  sky_zenith = { 0.3, 0.38, 0.62 }, stars = 0, moons = false }
  S.match()
  local E = require("rts.economy")
  local rand = B.rng(99)
  for _, c in ipairs({ M.SOUTH, M.NORTH }) do
    local s = c.z < 0 and 1 or -1
    E.add_mine(c.x - 60, c.z + s * 30)
    E.add_mine(c.x + 60, c.z + s * 30)
    for _ = 1, 30 do E.add_tree(c.x + rand(-160, 160), c.z - s * rand(40, 90), rand(0.8, 1.3)) end
  end
  -- the beacon on the hill: a stone tower with a fire basket
  local y = prts.scene.height(0, 0)
  B.box("stone", -2, y - 1, -2, 2, y + 9, 2)
  prts.scene.light{ pos = { 0, y + 10, 0 }, color = { 1.0, 0.6, 0.3 }, radius = 30 }
  prts.scene.marker("start", { M.SOUTH.x, prts.scene.height(M.SOUTH.x, M.SOUTH.z) + 0.1, M.SOUTH.z + 30 }, math.pi)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.SOUTH.x, z = M.SOUTH.z },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true, x = M.NORTH.x, z = M.NORTH.z },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -480, -480 }, max = { 480, 480 }, cell = 2 }
  require("rts.command").AREA = { -480, -480, 480, 480 }
  if opts and opts.no_match then return true end
  require("rts.match").start{ sides = M.SIDES, serfs = 4, custodians = "neutral", ruins = M.RUINS }
  local c = S.new_company(1, "Lanterns")
  for i = 1, 6 do S.spawn_unit(1, "watchfire.lantern_guard", M.SOUTH.x - 4 + i * 1.4, M.SOUTH.z + 40, c) end
  return true
end

prts.events.on("session_started", "maps.watchfire_hill", M.on_started)
return M
