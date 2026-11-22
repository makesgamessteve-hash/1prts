-- Reefholm: the Reef War (M8, story act VII). The storms Caldrith stirs up have brought the Brine
-- League north: their raft-city, the Tidehold, lies anchored beyond the reef of Reefholm, whose
-- lighthouse and pearl beds they mean to take, and the Imperium sky-barge sunk between the reef
-- and the market town of Saltwick holds charts of the sky lanes worth a fleet to whoever reads
-- them. The Crown holds the harbour town on the south shore, with a galleon, a caravel and a
-- diving bell on the jetty. The Crown wins when the Tidehold sinks (the League's only
-- headquarters here) and loses if its keep falls. MIT License.
local S = require("rts.state")
local B = require("lib.build")
local Wa = require("rts.water")

local M = {}
M.ID = "reefholm"
M.TOWN = { x = 0, z = -520 }
M.TIDEHOLD = { x = 0, z = 330 }
M.LIGHT = { x = 10, z = 40 }
M.MARKET = { x = 560, z = -40 }
M.WRECK = { x = 230, z = -150 }
M.BELL = { x = -70, z = -326 }
M.HALF = { x = 700, z = 700 }
M.FOG = { 180, 1000, { 0.62, 0.70, 0.78 } }
M.music = "music/proving_ground.ogg"

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.80, 0.88, 0.66 }, uv_scale = 4 }
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
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.22, 0.45, 0.58 }, uv_scale = 6 }
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
  -- a wide bay: the sea in the middle, the reef islands, the town to the south, Saltwick to the east
  local flats = { { x = 0, z = 0, radius = 470, height = -14, blend = 120 },
                  { x = M.TOWN.x, z = M.TOWN.z, radius = 110, height = 3, blend = 60 },
                  { x = M.MARKET.x, z = M.MARKET.z, radius = 80, height = 3, blend = 60 },
                  { x = M.LIGHT.x, z = M.LIGHT.z, radius = 26, height = 2, blend = 20 },
                  { x = -150, z = 70, radius = 22, height = 1.5, blend = 20 },
                  { x = 150, z = 120, radius = 20, height = 1.5, blend = 20 } }
  prts.scene.terrain{ size = 2048, spacing = 4, seed = 1212, height = 14, octaves = 4, material = "grass", flats = flats }
  prts.scene.env{
    sun_dir = { 0.45, 0.5, -0.4 }, sun_color = { 1.0, 0.88, 0.74 }, ambient = { 0.34, 0.37, 0.43 },
    fog_color = M.FOG[3], fog_near = M.FOG[1], fog_far = M.FOG[2],
    sky_horizon = { 0.74, 0.80, 0.88 }, sky_zenith = { 0.32, 0.46, 0.72 }, stars = 0, moons = false,
  }
  Wa.setup{ level = 0, area = { -1000, -1000, 1000, 1000 }, fog = M.FOG }
  S.match()
  local E = require("rts.economy")
  local rand = B.rng(1212)
  for _ = 1, 60 do E.add_tree(-250 + rand(-90, 90), -560 + rand(-40, 40), rand(0.8, 1.3)) end
  for _ = 1, 40 do E.add_tree(260 + rand(-60, 60), -580 + rand(-40, 40), rand(0.8, 1.3)) end
  E.add_mine(-120, -600)
  E.add_mine(120, -610)
  E.add_spring(-60, -600)
  E.add_pearl_bed(-140, 95)
  E.add_pearl_bed(170, 140)
  E.add_pearl_bed(40, 70)
  local N = require("rts.neutral")
  N.lighthouse{ name = "Reefholm light", x = M.LIGHT.x, z = M.LIGHT.z }
  N.market{ name = "Saltwick", x = M.MARKET.x, z = M.MARKET.z }
  require("props.wreck").spawn{ name = "sky-barge", x = M.WRECK.x, z = M.WRECK.z, yaw = 0.5 }
  -- the jetty out to the diving bell
  local bx, bz = M.BELL.x, M.BELL.z
  B.box("planks", bx - 1.5, 0.9, -410, bx + 1.5, 1.1, bz - 2.6)
  for z = -400, bz - 6, 8 do B.box("oak", bx - 1.4, -15, z, bx - 1.0, 0.9, z + 0.4) end -- its piles
  require("props.bell").make{ name = "bell", x = bx, z = bz, top = 1.1, yaw = math.pi } -- open towards the jetty
  prts.scene.marker("start", { M.TOWN.x, prts.scene.height(M.TOWN.x, M.TOWN.z) + 0.1, M.TOWN.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.TOWN.x, z = M.TOWN.z,
    gold = 700, timber = 600, aether = 100, powder = 120 },
  { id = 2, name = "The Brine League", faction = "brine", color = "red", ai = true, x = M.TIDEHOLD.x, z = M.TIDEHOLD.z,
    gold = 600, timber = 400, pearls = 100, aether = 120, no_hq = true },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -650, -650, 650, 650 }
  Wa.close_grid()
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local St = require("rts.structures")
  local Fl = require("rts.fleet")
  local sides = {}
  for i, s in ipairs(M.SIDES) do
    sides[i] = {}
    for k, v in pairs(s) do sides[i][k] = v end
  end
  match.start{ sides = sides, serfs = 4 }
  local town = M.TOWN
  local hold = S.hq(1)
  S.data(hold).queue, S.data(hold).train = { "hq_up" }, 1e6
  St.train_tick(hold, 0)
  St.place(1, "crown.harbor", town.x + 70, town.z + 70, true)
  St.place(1, "crown.watchtower", town.x - 60, town.z + 70, true)
  St.place(1, "crown.farmstead", town.x - 50, town.z - 40, true)
  St.place(1, "crown.farmstead", town.x + 50, town.z - 40, true)
  St.place(1, "crown.barracks", town.x + 55, town.z + 10, true)
  local foot = S.new_company(1, "Foot")
  for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", town.x - 10 + (i % 4) * 1.4, town.z + 40 + i // 4 * 1.4, foot) end
  Fl.spawn(1, "crown.war_galleon", 0, -360, math.pi, "ship.galleon")
  Fl.spawn(1, "crown.caravel", 60, -350, math.pi, "ship.caravel")
  -- the League: the Tidehold beyond the reef, its ships, harpooneers on the lighthouse's island
  local th = require("props.tidehold").spawn(2, M.TIDEHOLD.x, M.TIDEHOLD.z)
  S.data(th).queue = {}
  Fl.spawn(2, "brine.brigantine", -60, 250, 0, "ship.brigantine")
  Fl.spawn(2, "brine.sloop", 60, 240, 0, "ship.sloop.1")
  Fl.spawn(2, "brine.sloop", 120, 260, 0, "ship.sloop.2")
  local hp = S.new_company(2, "Harpooneers")
  for i = 1, 6 do S.spawn_unit(2, "brine.harpooneers", M.LIGHT.x - 6 + i * 1.5, M.LIGHT.z + 12, hp) end
  S.side(2).expanding = true
  return true
end

-- The League's ships go for the nearest Crown ship every ten seconds.
function M.tick(now)
  if require("game.session").current ~= M.ID or not prts.entity.find(S.REGISTRY) then return end
  if now % 300 ~= 0 then return end
  local Fl = require("rts.fleet")
  for _, e in ipairs(Fl.ships()) do
    local d = S.data(e)
    if S.alive(e) and d.side == 2 and not d.sinking then
      local x, _, z = S.pos(e)
      local t = Fl.enemy_ship(2, x, z, 500)
      if t then
        local tx, _, tz = S.pos(t)
        Fl.order(e, tx, tz)
      end
    end
  end
end

prts.events.on("session_started", "maps.reefholm", M.on_started)
prts.events.on("tick", "maps.reefholm", M.tick)
return M
