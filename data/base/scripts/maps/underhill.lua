-- Underhill: Under the Hill (M7, story act VI). Below Merewatch Hill the Gloam Courts have opened
-- their sealed gates for the first time since the Skyfall: Caldrith's broken chain runs down out
-- of the rock into a glowstone cavern under the hill, and the Durrow mean to keep what lies at
-- its end. The Crown holds the hill, with a deep shaft already sunk into the caverns and a gate
-- into the Gloam's tunnel road at its foot. The road runs east under the field, past the chain's
-- cavern, to the Hollow Court, whose gatehouse stands on the far side of the field. Three minutes
-- in, a tunnel wyrm bursts up in the Crown's camp. The Crown wins when the Hollow Court falls,
-- and loses if its keep falls. MIT License.
local S = require("rts.state")
local B = require("lib.build")
local Tu = require("rts.tunnels")

local M = {}
M.ID = "underhill"
M.CAMP = { x = -262, z = -62 }
M.COURT = { x = 302, z = 2 }
M.SHAFT = { x = -102, z = 62 }
M.HALF = { x = 620, z = 500 }
M.WYRM_AT = 180 -- seconds into the match when the wyrm bursts up in the camp
M.music = "music/proving_ground.ogg"
M.FOG = { 180, 950, { 0.62, 0.66, 0.70 } }

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.74, 0.86, 0.62 }, uv_scale = 4 }
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
  m{ id = "sail", texture = "textures/plaster.png", tint = { 1.1, 1.06, 0.95 }, uv_scale = 2 }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  require("props.horse").materials()
end

local function forest(rand, x, z, w, d, n)
  local E = require("rts.economy")
  for _ = 1, n do E.add_tree(x + rand(-w, w), z + rand(-d, d), rand(0.8, 1.3)) end
end

-- Caldrith's broken chain: great iron links running down out of the cavern's roof.
local function chain(n)
  local e = prts.entity.spawn{ name = "caldrith.chain", pos = { n.x, n.y, n.z } }
  local h = n.size[3]
  for i = 0, 9 do
    local f = i / 9
    local x, y, z = -14 + f * 20, h + 6 - f * (h + 4), -10 + f * 12
    if i % 2 == 0 then
      prts.scene.box{ min = { x - 1.6, y - 0.4, z - 0.4 }, max = { x + 1.6, y + 0.4, z + 0.4 }, material = "iron", entity = e }
      prts.scene.box{ min = { x - 1.6, y - 2.4, z - 0.4 }, max = { x - 1.0, y + 0.4, z + 0.4 }, material = "iron", entity = e }
      prts.scene.box{ min = { x + 1.0, y - 2.4, z - 0.4 }, max = { x + 1.6, y + 0.4, z + 0.4 }, material = "iron", entity = e }
    else
      prts.scene.box{ min = { x - 0.4, y - 0.4, z - 1.6 }, max = { x + 0.4, y + 0.4, z + 1.6 }, material = "iron", entity = e }
      prts.scene.box{ min = { x - 0.4, y - 2.4, z - 1.6 }, max = { x + 0.4, y + 0.4, z - 1.0 }, material = "iron", entity = e }
    end
  end
  prts.scene.zone("chain", { n.x - 18, n.y, n.z - 18 }, { n.x + 18, n.y + h, n.z + 18 })
  return e
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 1600, spacing = 4, seed = 616, height = 18, octaves = 4, material = "grass", flats = {
    { x = M.CAMP.x, z = M.CAMP.z, radius = 90, height = 26, blend = 90 },
    { x = M.COURT.x, z = M.COURT.z, radius = 90, height = 6, blend = 60 },
    { x = 0, z = 0, radius = 140, height = 4, blend = 120 } } }
  prts.scene.env{
    sun_dir = { -0.3, 0.6, -0.5 }, sun_color = { 0.96, 0.86, 0.72 }, ambient = { 0.32, 0.34, 0.40 },
    fog_color = M.FOG[3], fog_near = M.FOG[1], fog_far = M.FOG[2],
    sky_horizon = { 0.74, 0.78, 0.84 }, sky_zenith = { 0.34, 0.46, 0.70 }, stars = 0, moons = false,
  }
  S.match()
  -- the tunnel road: the gate at the hill's foot, a junction, the chain's cavern, a junction, and
  -- (at match start) the gallery on to the Hollow Court's hall; the Crown's deep shaft drops in
  Tu.setup{ fog = M.FOG, rock = "rock" }
  Tu.node("west", { x = -198, z = 2, depth = 12, kind = "mouth", dir = "w" })
  Tu.node("j1", { x = -102, z = 2, depth = 16 })
  Tu.node("chain", { x = 2, z = 2, depth = 44, kind = "cavern", size = { 40, 40, 18 }, grotto = true })
  Tu.node("j2", { x = 106, z = 2, depth = 16 })
  Tu.node("shaft", { x = M.SHAFT.x, z = M.SHAFT.z, depth = 40, kind = "shaft" })
  Tu.link("road.1", "west", "j1")
  Tu.link("road.2", "j1", "chain")
  Tu.link("road.3", "chain", "j2")
  Tu.link("shaft.road", "shaft", "j1")
  Tu.build()
  chain(Tu.data().nodes.chain)
  local E = require("rts.economy")
  local rand = B.rng(616)
  forest(rand, -300, -260, 120, 60, 70)
  forest(rand, -360, 180, 80, 80, 50)
  forest(rand, 320, 250, 100, 60, 40)
  E.add_mine(-330, 20)
  E.add_mine(-200, -150)
  E.add_mine(380, -80)
  E.add_quarry(360, 90)
  E.add_quarry(250, -110)
  E.add_grotto(250, 96)
  E.add_spring(-300, -150)
  prts.scene.marker("start", { M.CAMP.x, prts.scene.height(M.CAMP.x, M.CAMP.z) + 0.1, M.CAMP.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.CAMP.x, z = M.CAMP.z,
    gold = 700, timber = 500, aether = 120, powder = 40 },
  { id = 2, name = "The Hollow Court", faction = "gloam", color = "red", ai = true, x = M.COURT.x, z = M.COURT.z,
    gold = 500, stone = 500, glowstone = 60, aether = 80 },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -560, -420, 560, 420 }
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local St = require("rts.structures")
  local sides = {}
  for i, s in ipairs(M.SIDES) do
    sides[i] = {}
    for k, v in pairs(s) do sides[i][k] = v end
  end
  match.start{ sides = sides, serfs = 4 }
  local camp, court = M.CAMP, M.COURT
  -- the Crown on Merewatch Hill: a keep, the deep shaft, a barracks, farms and a tower
  local hold = S.hq(1)
  S.data(hold).queue, S.data(hold).train = { "hq_up" }, 1e6
  St.train_tick(hold, 0)
  St.place(1, "crown.deep_shaft", M.SHAFT.x, M.SHAFT.z, true)
  St.place(1, "crown.barracks", camp.x + 50, camp.z + 10, true)
  St.place(1, "crown.farmstead", camp.x - 50, camp.z - 30, true)
  St.place(1, "crown.farmstead", camp.x - 50, camp.z + 30, true)
  St.place(1, "crown.watchtower", camp.x + 60, camp.z + 60, true)
  for _, label in ipairs({ "Foot", "Bows" }) do
    local c = S.new_company(1, label)
    local unit = label == "Foot" and "crown.men_at_arms" or "crown.crossbowmen"
    for i = 1, 8 do S.spawn_unit(1, unit, camp.x + 20 + (i % 4) * 1.4 + (label == "Bows" and 8 or 0), camp.z + 30 + i // 4 * 1.4, c) end
  end
  -- the Hollow Court across the field, its hall under the ground joined to the tunnel road
  local court_hq = S.hq(2)
  S.data(court_hq).queue, S.data(court_hq).train = { "hq_up" }, 1e6
  St.train_tick(court_hq, 0)
  local hall = Tu.hall(S.hq(2), 18)
  Tu.link("road.4", "j2", hall.id)
  Tu.build_part({ "j2", hall.id }, { "road.4" })
  St.place(2, "gloam.guard_hall", court.x + 40, court.z + 50, true)
  St.place(2, "gloam.stonecutters", court.x - 50, court.z - 80, true)
  St.place(2, "gloam.mycel_gardens", court.x + 50, court.z - 40, true)
  St.place(2, "gloam.mycel_gardens", court.x - 45, court.z + 45, true)
  St.place(2, "gloam.glow_tower", court.x - 70, court.z, true)
  St.place(2, "gloam.echo_pillar", court.x - 90, court.z + 20, true)
  St.place(2, "gloam.wyrm_hollow", court.x + 70, court.z + 10, true)
  S.side(2).expanding = true
  for _, label in ipairs({ "Guard", "Slingers" }) do
    local c = S.new_company(2, label)
    local unit = label == "Guard" and "gloam.basalt_guard" or "gloam.slinger"
    for i = 1, 8 do S.spawn_unit(2, unit, court.x - 30 + (i % 4) * 1.4 + (label == "Slingers" and 8 or 0), court.z + 20 + i // 4 * 1.4, c) end
  end
  -- the wyrm, harnessed, with a company of basalt guard in its gondola
  local Wy = require("props.wyrm")
  local wyrm = Wy.spawn(2, court.x + 70, court.z + 50, "wyrm.court")
  S.cargo(wyrm, 2, "gloam.basalt_guard", "Wyrm guard", 8)
  S.match().underhill = { wyrm = M.WYRM_AT * 30 + prts.time.tick() }
  S.dispatch(1, "Tam Durran", "The gate at the hill's foot stands open. It was sealed this morning. Something came up.")
  return true
end

-- Three minutes in, the wyrm goes under the field and bursts up in the Crown's camp.
function M.tick(now)
  if require("game.session").current ~= M.ID or not prts.entity.find(S.REGISTRY) then return end
  local st = S.match().underhill
  if not st or not st.wyrm or now < st.wyrm then return end
  st.wyrm = nil
  local w = prts.entity.find("wyrm.court")
  if w and require("props.wyrm").order(w, M.CAMP.x + 40, M.CAMP.z + 40) then
    S.dispatch(1, "Lookout", "The ground is shaking under the camp!")
  end
end

prts.events.on("session_started", "maps.underhill", M.on_started)
prts.events.on("tick", "maps.underhill", M.tick)
return M
