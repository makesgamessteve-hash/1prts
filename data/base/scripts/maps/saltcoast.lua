-- Saltcoast: the Harbor Siege (M5, story act IV). Clan Varrak besieges the Crown's harbor town of
-- Saltmere from a camp across the bay: a war lodge, a raft-yard with longships and a ram galley,
-- wyvern riders on the cliffs. The relief comes up the bay from the east, a war galleon, a caravel
-- and two cogs with four companies, and over the hills by skyship from the mast behind the town.
-- The commander starts in the galleon's chart room. The siege is broken when the war lodge falls;
-- it is lost if the town's keep falls or the galleon sinks. MIT License.
local S = require("rts.state")
local B = require("lib.build")
local Wa = require("rts.water")

local M = {}
M.ID = "saltcoast"
M.TOWN = { x = 150, z = -330 }
M.CAMP = { x = 150, z = 330 }
M.FLEET = { x = 760, z = 0 }
M.MAST = { x = 40, z = -420 }
M.HALF = { x = 600, z = 500 }
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
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.25, 0.42, 0.62 }, uv_scale = 6 }
  m{ id = "sail", texture = "textures/plaster.png", tint = { 1.1, 1.06, 0.95 }, uv_scale = 2 }
  m{ id = "sail_red", texture = "textures/plaster.png", tint = { 1.2, 0.5, 0.4 }, uv_scale = 2 }
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

-- The mast behind the town: a timber tower with a lift cage up to the skyship's deck.
local function mast()
  local Lift = require("props.lift")
  local x, z = M.MAST.x, M.MAST.z
  local g = prts.scene.height(x, z)
  for _, c in ipairs({ { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } }) do
    B.box("oak", x + c[1] - 0.3, g - 1, z + c[2] - 0.3, x + c[1] + 0.3, g + 46, z + c[2] + 0.3)
  end
  B.box("planks", x - 2.5, g + 44.8, z - 6, x + 2.5, g + 45, z - 2.2)      -- the landing towards the ship
  B.box("planks", x - 2.5, g + 44.8, z + 2.2, x + 2.5, g + 45, z + 2.5)
  Lift.make{ name = "mast.lift", pos = { x, g + 0.25, z }, floors = { g + 0.25, g + 45 }, size = { 3.6, 3.6 },
             gate = x - 2.1, material = "planks", gate_material = "iron", lever_material = "oak" }
  prts.scene.marker("mast_foot", { x + 4, g + 0.1, z + 4 }, 0)
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  -- the bay: a channel from the east into a round harbor basin, with land north and south
  local flats = { { x = M.TOWN.x, z = M.TOWN.z, radius = 110, height = 3, blend = 60 },
                  { x = M.CAMP.x, z = M.CAMP.z, radius = 120, height = 3, blend = 60 } }
  for _, x in ipairs({ 0, 150, 300, 450, 600, 750, 900, 1050 }) do
    flats[#flats + 1] = { x = x, z = 0, radius = 190, height = -10, blend = 70 }
  end
  prts.scene.terrain{ size = 2600, spacing = 4, seed = 404, height = 24, octaves = 4, material = "grass", flats = flats }
  prts.scene.env{
    sun_dir = { 0.5, 0.45, -0.4 }, sun_color = { 1.0, 0.86, 0.70 }, ambient = { 0.34, 0.36, 0.42 },
    fog_color = { 0.64, 0.70, 0.78 }, fog_near = 200, fog_far = 1100,
    sky_horizon = { 0.76, 0.82, 0.90 }, sky_zenith = { 0.34, 0.48, 0.74 }, stars = 0, moons = false,
  }
  Wa.setup{ level = 0, area = { -1300, -1300, 1300, 1300 } }
  S.match()
  local E = require("rts.economy")
  local rand = B.rng(55)
  forest(rand, -250, -320, 80, 120, 60)
  forest(rand, -250, 320, 80, 120, 60)
  forest(rand, 400, 420, 120, 40, 40)
  forest(rand, 400, -440, 120, 40, 40)
  E.add_mine(60, -380)
  E.add_mine(250, -420)
  E.add_mine(60, 400)
  E.add_mine(250, 420)
  mast()
  -- where the commander first stands (moved to the galleon's chart room once the match starts)
  prts.scene.marker("start", { M.TOWN.x, prts.scene.height(M.TOWN.x, M.TOWN.z) + 0.1, M.TOWN.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.TOWN.x, z = M.TOWN.z,
    gold = 600, timber = 600, powder = 100 },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true, x = M.CAMP.x, z = M.CAMP.z,
    gold = 300, timber = 300, bone = 60 },
}

-- Adds a company of `n` men of `unit` to a ship's cargo (they wait aboard to be landed).
M.cargo = function(...) return S.cargo(...) end

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -600, -435, 1000, 435 } -- what the war table shows: the bay and its mouth
  Wa.close_grid()
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local St = require("rts.structures")
  local Fl = require("rts.fleet")
  local Sh = require("props.skyship")
  local Tools = require("rts.tools")
  local sides = {}
  for i, s in ipairs(M.SIDES) do
    sides[i] = {}
    for k, v in pairs(s) do sides[i][k] = v end
  end
  match.start{ sides = sides, serfs = 4 }
  local town, camp = M.TOWN, M.CAMP
  -- the town: the holdfast is a keep already, with a harbor, towers, farms and a balloon
  local hold = S.hq(1)
  S.data(hold).queue, S.data(hold).train = { "hq_up" }, 1e6
  St.train_tick(hold, 0)
  St.place(1, "crown.harbor", town.x, town.z + 86, true)
  St.place(1, "crown.watchtower", town.x - 60, town.z + 60, true)
  St.place(1, "crown.watchtower", town.x + 60, town.z + 60, true)
  St.place(1, "crown.farmstead", town.x - 50, town.z - 40, true)
  St.place(1, "crown.farmstead", town.x + 50, town.z - 40, true)
  St.place(1, "crown.barracks", town.x + 55, town.z + 10, true)
  St.place(1, "crown.balloon", town.x - 55, town.z + 10, true)
  for _, label in ipairs({ "Foot", "Bows" }) do
    local c = S.new_company(1, label)
    local unit = label == "Foot" and "crown.men_at_arms" or "crown.crossbowmen"
    for i = 1, 8 do S.spawn_unit(1, unit, town.x - 10 + (i % 4) * 1.4 + (label == "Bows" and 14 or 0), town.z + 30 + i // 4 * 1.4, c) end
  end
  -- the siege camp across the bay
  local lodge = S.hq(2)
  S.data(lodge).queue, S.data(lodge).train = { "hq_up" }, 1e6
  St.train_tick(lodge, 0)
  local yard = St.place(2, "wildborn.raft_yard", camp.x, camp.z - 80, true)
  S.data(yard).seaward = -1
  St.place(2, "wildborn.war_pit", camp.x + 50, camp.z + 10, true)
  St.place(2, "wildborn.herd_pen", camp.x - 50, camp.z + 30, true)
  St.place(2, "wildborn.herd_pen", camp.x - 50, camp.z - 10, true)
  St.place(2, "wildborn.totem_watch", camp.x - 60, camp.z - 60, true)
  St.place(2, "wildborn.totem_watch", camp.x + 60, camp.z - 60, true)
  St.place(2, "wildborn.roost_cliffs", camp.x + 70, camp.z + 60, true)
  S.side(2).expanding = true -- the siege holds its ground
  for _, label in ipairs({ "Brutes", "Spears" }) do
    local c = S.new_company(2, label)
    local unit = label == "Brutes" and "wildborn.ghorr" or "wildborn.spearhurler"
    for i = 1, 6 do S.spawn_unit(2, unit, camp.x - 10 + (i % 3) * 1.5 + (label == "Spears" and 12 or 0), camp.z + 40 + i // 3 * 1.5, c) end
  end
  local wings = S.new_company(2, "Wyverns")
  for i = 1, 3 do S.spawn_unit(2, "wildborn.wyvern", camp.x + 60 + i * 4, camp.z + 70, wings) end
  Fl.spawn(2, "wildborn.longship", camp.x - 40, camp.z - 140, 0, "ship.longship.1")
  Fl.spawn(2, "wildborn.longship", camp.x + 40, camp.z - 140, 0, "ship.longship.2")
  Fl.spawn(2, "wildborn.ram_galley", camp.x + 120, camp.z - 150, 0, "ship.ram_galley")
  -- the relief fleet at the bay's mouth, heading west
  local f = M.FLEET
  local galleon = Fl.spawn(1, "crown.war_galleon", f.x, f.z, math.pi / 2, "ship.galleon")
  Fl.spawn(1, "crown.caravel", f.x + 30, f.z - 50, math.pi / 2, "ship.caravel")
  local cog1 = Fl.spawn(1, "crown.cog", f.x + 60, f.z + 45, math.pi / 2, "ship.cog.1")
  local cog2 = Fl.spawn(1, "crown.cog", f.x + 90, f.z - 30, math.pi / 2, "ship.cog.2")
  M.cargo(cog1, 1, "crown.men_at_arms", "Foot", 8)
  M.cargo(cog1, 1, "crown.crossbowmen", "Bows", 8)
  M.cargo(cog2, 1, "crown.men_at_arms", "Foot", 8)
  M.cargo(cog2, 1, "crown.knight", "Knights", 4)
  -- the skyship at the mast
  local g = prts.scene.height(M.MAST.x, M.MAST.z)
  local sky = Sh.spawn("skyship", M.MAST.x, M.MAST.z - 14, 0)
  prts.entity.place(sky, { M.MAST.x, g + 45, M.MAST.z - 14.5 }, 0)
  -- the commander in the galleon's chart room, at its war table
  local plan = prts.entity.data(galleon).plan
  local r = plan.rooms["chart room"]
  local base = { (r.min[1] + r.max[1]) / 2, r.min[2], (r.min[3] + r.max[3]) / 2 - 0.4 }
  prts.entity.data(S.hq(1)).furnished = true -- the town keep has its own table; the chart room is the post now
  Tools.furnish(galleon, base)
  local Deck = require("lib.deckplan")
  local x, y, z = Deck.to_world(galleon, base[1], base[2] + 0.05, base[3] + 2.4)
  prts.commander.place({ x, y, z }, math.pi / 2)
  return true
end

-- The Wildborn's ships go for the nearest Crown ship every ten seconds; the end conditions.
function M.tick(now)
  if require("game.session").current ~= M.ID or not prts.entity.find(S.REGISTRY) then return end
  local Fl = require("rts.fleet")
  if now % 300 == 0 then
    for _, e in ipairs(Fl.ships()) do
      local d = S.data(e)
      if S.alive(e) and d.side == 2 and not d.sinking then
        local x, _, z = S.pos(e)
        local t = Fl.enemy_ship(2, x, z, 700)
        if t then
          local tx, _, tz = S.pos(t)
          Fl.order(e, tx, tz)
        end
      end
    end
    local wings = nil
    for _, c in ipairs(require("rts.companies").of_side(2)) do
      if c.label == "Wyverns" then wings = c end
    end
    if wings and wings.order.kind ~= "attack" then
      require("rts.companies").order(wings, { kind = "attack", x = M.TOWN.x, z = M.TOWN.z })
    end
  end
end

prts.events.on("rts_ship_sunk", "maps.saltcoast", function(e, side)
  if prts.entity.name(e) == "ship.galleon" and side == 1 then
    S.side(1).defeated = true
    S.dispatch(1, "Herald", "The galleon is lost with all hands. The relief has failed.")
  end
end)
prts.events.on("session_started", "maps.saltcoast", M.on_started)
prts.events.on("tick", "maps.saltcoast", M.tick)
return M
