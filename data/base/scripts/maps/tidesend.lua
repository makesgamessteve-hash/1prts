-- Tidesend: a four-faction skirmish (M8) on a 2.4 km square around a round sea. The Crown holds
-- the south-west (the player), Clan Varrak the north-west, the Hollow Court the north-east, and
-- the Brine League's Wharf hall stands on the sea's south-east shore. A lighthouse on the isle in
-- the middle of the sea and the market town of Tidesend on the south shore are there for the
-- taking. Every side fights every other; the last one standing wins. MIT License.
local S = require("rts.state")
local E = require("rts.economy")
local B = require("lib.build")
local Wa = require("rts.water")

local M = {}
M.ID = "tidesend"
M.HALF = { x = 900, z = 900 }
M.SEA = 420
M.FOG = { 220, 1400, { 0.66, 0.72, 0.78 } }
M.SPOTS = {
  crown = { x = -650, z = 650 }, wildborn = { x = -650, z = -650 }, gloam = { x = 650, z = -650 },
  brine = { x = 330, z = 330 },
}
M.MARKET = { x = -40, z = 620 }
M.music = "music/proving_ground.ogg"

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
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.24, 0.46, 0.6 }, uv_scale = 6 }
  m{ id = "sail", texture = "textures/plaster.png", tint = { 1.1, 1.06, 0.95 }, uv_scale = 2 }
  m{ id = "sail_red", texture = "textures/plaster.png", tint = { 1.2, 0.5, 0.4 }, uv_scale = 2 }
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
  local flats = { { x = 0, z = 0, radius = M.SEA, height = -12, blend = 60 }, { x = 0, z = 0, radius = 24, height = 2, blend = 16 },
                  { x = M.MARKET.x, z = M.MARKET.z, radius = 70, height = 4, blend = 50 } }
  for name, c in prts.sorted_pairs(M.SPOTS) do
    flats[#flats + 1] = { x = c.x, z = c.z, radius = name == "brine" and 60 or 140, height = 4, blend = name == "brine" and 30 or 90 }
  end
  prts.scene.terrain{ size = 2400, spacing = 4, seed = 4040, height = 22, octaves = 4, material = "grass", flats = flats }
  prts.scene.env{
    sun_dir = { 0.35, 0.6, -0.45 }, sun_color = { 1.0, 0.9, 0.76 }, ambient = { 0.34, 0.36, 0.42 },
    fog_color = M.FOG[3], fog_near = M.FOG[1], fog_far = M.FOG[2],
    sky_horizon = { 0.76, 0.80, 0.86 }, sky_zenith = { 0.36, 0.48, 0.74 }, stars = 0, moons = false,
  }
  Wa.setup{ level = 0, area = { -1200, -1200, 1200, 1200 }, fog = M.FOG }
  S.match()
  local rand = B.rng(4040)
  for name, c in prts.sorted_pairs(M.SPOTS) do
    local sx, sz = c.x < 0 and 1 or -1, c.z < 0 and 1 or -1
    if name == "brine" then sx, sz = 1, 1 end -- its hinterland lies away from the sea
    E.add_mine(c.x + sx * 70, c.z + sz * 25)
    E.add_mine(c.x + sx * 25, c.z + sz * 80)
    E.add_spring(c.x + sx * 60, c.z + sz * 70)
    for _ = 1, 50 do E.add_tree(c.x + sx * 140 + rand(-50, 50), c.z + sz * 40 + rand(-70, 70), rand(0.8, 1.3)) end
  end
  local g = M.SPOTS.gloam
  E.add_quarry(g.x - 100, g.z + 10)
  E.add_grotto(g.x - 80, g.z + 90)
  local b = M.SPOTS.brine
  E.add_pearl_bed(b.x - 80, b.z - 60)
  E.add_pearl_bed(b.x - 30, b.z - 110)
  local N = require("rts.neutral")
  N.lighthouse{ name = "Tidesend light", x = 0, z = 0 }
  N.market{ name = "Tidesend", x = M.MARKET.x, z = M.MARKET.z }
  local c = M.SPOTS.crown
  prts.scene.marker("start", { c.x, prts.scene.height(c.x, c.z) + 0.1, c.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.SPOTS.crown.x,
    z = M.SPOTS.crown.z, gold = 500, timber = 400 },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true, x = M.SPOTS.wildborn.x,
    z = M.SPOTS.wildborn.z, gold = 500, timber = 400 },
  { id = 3, name = "The Hollow Court", faction = "gloam", color = "red", ai = true, x = M.SPOTS.gloam.x,
    z = M.SPOTS.gloam.z, gold = 500, stone = 400 },
  { id = 4, name = "The Brine League", faction = "brine", color = "red", ai = true, x = M.SPOTS.brine.x,
    z = M.SPOTS.brine.z, gold = 500, timber = 400, pearls = 50 },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -880, -880, 880, 880 }
  Wa.close_grid()
  if opts and opts.no_match then return true end
  require("rts.match").start{ sides = M.SIDES, serfs = 5 }
  -- the League starts with two pearl divers at the beds
  local b = M.SPOTS.brine
  for i = 1, 2 do
    local diver = S.spawn_unit(4, "brine.pearl_diver", b.x + 14 + i * 2, b.z + 14)
    require("rts.economy").gather(diver, "pearls")
  end
  return true
end

prts.events.on("session_started", "maps.tidesend", M.on_started)
return M
