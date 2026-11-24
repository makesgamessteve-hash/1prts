-- Hollowfell: a four-way skirmish (M7) on a 2 km square of downs: the Crown in the south-west
-- (the player), Clan Varrak in the north-west, the Hollow Court of the Gloam in the north-east
-- and a rival Crown host, Lord Varric's, in the south-east. Every side fights every other; the
-- last one standing wins. Under the middle lies a glowstone cavern with a tunnel road out to four
-- gates. MIT License.
local S = require("rts.state")
local E = require("rts.economy")
local B = require("lib.build")
local Tu = require("rts.tunnels")

local M = {}
M.ID = "hollowfell"
M.HALF = { x = 800, z = 800 }
M.RUINS = { { x = 300, z = 0 }, { x = -300, z = 0 } } -- ruins of the Imperium (the Custodians' option)
M.CORNERS = {
  { x = -542, z = 542 }, { x = -542, z = -542 }, { x = 542, z = -542 }, { x = 542, z = 542 },
}
M.music = "music/proving_ground.ogg"
M.FOG = { 200, 1300, { 0.66, 0.70, 0.74 } }

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.78, 0.9, 0.64 }, uv_scale = 4 }
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
  m{ id = "rock", texture = "textures/rock.png", tint = { 0.78, 0.76, 0.8 }, uv_scale = 2 }
  m{ id = "iron", texture = "textures/iron.png", uv_scale = 1 }
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.3, 0.45, 0.6 }, uv_scale = 4 }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  require("props.horse").materials()
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  local flats = { { x = 0, z = 0, radius = 200, height = 2, blend = 150 } }
  for _, c in ipairs(M.CORNERS) do flats[#flats + 1] = { x = c.x, z = c.z, radius = 150, height = 4, blend = 90 } end
  -- a sinkhole over the cavern in the middle (the ground there is open: a hole made with the terrain)
  prts.scene.terrain{ size = 2048, spacing = 4, seed = 1717, height = 22, octaves = 4, material = "grass", flats = flats,
                      holes = { { 0, 0, 4, 4 } } }
  prts.scene.env{
    sun_dir = { 0.35, 0.6, -0.45 }, sun_color = { 1.0, 0.9, 0.76 }, ambient = { 0.34, 0.36, 0.42 },
    fog_color = M.FOG[3], fog_near = M.FOG[1], fog_far = M.FOG[2],
    sky_horizon = { 0.76, 0.80, 0.86 }, sky_zenith = { 0.36, 0.48, 0.74 }, stars = 0, moons = false,
  }
  S.match()
  local rand = B.rng(1717)
  for i, c in ipairs(M.CORNERS) do
    local sx, sz = c.x < 0 and 1 or -1, c.z < 0 and 1 or -1
    E.add_mine(c.x + sx * 70, c.z + sz * 20)
    E.add_mine(c.x + sx * 20, c.z + sz * 80)
    E.add_spring(c.x + sx * 60, c.z + sz * 70)
    E.add_quarry(c.x + sx * 100, c.z - sz * 10)
    if i == 4 then E.add_grotto(c.x + sx * 90, c.z + sz * 60) end
    for _ = 1, 50 do E.add_tree(c.x - sx * 80 + rand(-40, 40), c.z + sz * 130 + rand(-60, 60), rand(0.8, 1.3)) end
  end
  E.add_mine(0, 60)
  E.add_mine(60, -40)
  E.add_spring(-50, -30)
  -- the tunnel road under the middle: a glowstone cavern and four gates out onto the downs
  Tu.setup{ fog = M.FOG, rock = "rock" }
  Tu.node("deep", { x = 2, z = 2, depth = 30, kind = "cavern", size = { 32, 32, 12 }, grotto = true })
  for _, g in ipairs({ { "n", 2, -298 }, { "s", 2, 302 }, { "w", -298, 2 }, { "e", 302, 2 } }) do
    Tu.node("gate." .. g[1], { x = g[2], z = g[3], depth = 12, kind = "mouth", dir = g[1] })
    Tu.link("road." .. g[1], "deep", "gate." .. g[1])
  end
  Tu.build()
  local c = M.CORNERS[1]
  prts.scene.marker("start", { c.x, prts.scene.height(c.x, c.z) + 0.1, c.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false,
    x = M.CORNERS[1].x, z = M.CORNERS[1].z, gold = 500, timber = 400 },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true,
    x = M.CORNERS[2].x, z = M.CORNERS[2].z, gold = 500, timber = 400 },
  { id = 3, name = "Lord Varric's host", faction = "crown", color = "red", ai = true,
    x = M.CORNERS[3].x, z = M.CORNERS[3].z, gold = 500, timber = 400 },
  { id = 4, name = "The Hollow Court", faction = "gloam", color = "red", ai = true,
    x = M.CORNERS[4].x, z = M.CORNERS[4].z, gold = 500, stone = 400 },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -760, -760, 760, 760 }
  if not (opts and opts.no_match) then require("rts.match").start{ sides = M.SIDES, serfs = 5, custodians = require("game.settings").game().custodians, ruins = M.RUINS } end
  return true
end

prts.events.on("session_started", "maps.hollowfell", M.on_started)
return M
