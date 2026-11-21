-- Greyfen March: the proving ground of M3 (Ride Out). A grey-green march between fen and hills.
-- The Crown's camp in the west has stables, saddled horses, a supply wagon on its road and a
-- company of knights. In the east stands Lord Varric's walled fort, with a siege tower already
-- rolled up in front of its west wall. A rock hill in the north-west holds a gold mine with
-- galleries and a lift cage. MIT License.
local S = require("rts.state")
local B = require("lib.build")
local Horse = require("props.horse")
local Wagon = require("props.wagon")
local Tower = require("props.siege_tower")
local Mine = require("props.mine")

local M = {}
M.ID = "greyfen_march"
M.CAMP = { x = -200, z = 0 }
M.FORT = { x0 = 120, x1 = 200, z0 = -40, z1 = 40, thick = 3, height = 7 }
M.TOWER = { x = 95, z = 0 }
M.MINE = { x = -150, z = -110 }
M.ROAD = { { -250, -60 }, { -150, -60 }, { -150, 60 }, { -250, 60 } }
M.HALF = { x = 300, z = 200 }
M.music = "music/proving_ground.ogg"

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.78, 0.9, 0.72 }, uv_scale = 4 }
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
  m{ id = "gold_vein", texture = "textures/rock.png", tint = { 1.8, 1.4, 0.45 }, uv_scale = 1, emissive = true }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  Horse.materials()
end

-- The rival's fort: four curtain walls 7 m high with a wall-walk, merlons on the outer edge
-- (an open stretch on the west wall where the siege tower's bridge lands), square corner towers
-- and a gate in the north wall.
local function fort()
  local F = M.FORT
  local g = prts.scene.height((F.x0 + F.x1) / 2, (F.z0 + F.z1) / 2)
  local top = g + F.height
  local t = F.thick
  B.box("stone", F.x0, g - 1, F.z0, F.x0 + t, top, F.z1)                 -- west
  B.box("stone", F.x1 - t, g - 1, F.z0, F.x1, top, F.z1)                 -- east
  B.box("stone", F.x0, g - 1, F.z1 - t, F.x1, top, F.z1)                 -- south
  B.box("stone", F.x0, g - 1, F.z0, (F.x0 + F.x1) / 2 - 3, top, F.z0 + t) -- north, with the gate
  B.box("stone", (F.x0 + F.x1) / 2 + 3, g - 1, F.z0, F.x1, top, F.z0 + t)
  B.box("stone", (F.x0 + F.x1) / 2 - 3, g + 4, F.z0, (F.x0 + F.x1) / 2 + 3, top, F.z0 + t)
  -- merlons along the outer edges (the west wall leaves z -2..2 open for the bridge)
  local function merlons(x0, x1, z0, z1, along_x, skip)
    local a0, a1 = along_x and x0 or z0, along_x and x1 or z1
    for a = a0 + 0.6, a1 - 1.2, 2.4 do
      if not (skip and a + 1.2 > skip[1] and a < skip[2]) then
        if along_x then B.box("stone", a, top, z0, a + 1.2, top + 1.1, z1)
        else B.box("stone", x0, top, a, x1, top + 1.1, a + 1.2) end
      end
    end
  end
  merlons(F.x0, F.x0 + 0.6, F.z0, F.z1, false, { -2, 2 })
  merlons(F.x1 - 0.6, F.x1, F.z0, F.z1, false)
  merlons(F.x0, F.x1, F.z1 - 0.6, F.z1, true)
  merlons(F.x0, F.x1, F.z0, F.z0 + 0.6, true)
  for _, c in ipairs({ { F.x0, F.z0 }, { F.x1, F.z0 }, { F.x0, F.z1 }, { F.x1, F.z1 } }) do
    B.box("stone", c[1] - 4, g - 1, c[2] - 4, c[1] + 4, top + 4, c[2] + 4)
  end
  -- stairs from the ward up to the west wall-walk
  B.stairs("stone", { axis = "z", sign = 1, start = -20, w0 = F.x0 + t, w1 = F.x0 + t + 1.6, base = g,
                      rise = F.height / 24, run = 0.5, steps = 24, solid = true })
  return top
end

local function forest(rand, x, z, w, d, n)
  local E = require("rts.economy")
  for _ = 1, n do E.add_tree(x + rand(-w, w), z + rand(-d, d), rand(0.8, 1.3)) end
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  prts.scene.terrain{ size = 1024, spacing = 4, seed = 53, height = 18, octaves = 4, material = "grass",
                      flats = { { x = M.CAMP.x, z = M.CAMP.z, radius = 110, height = 1, blend = 60 },
                                { x = 130, z = 0, radius = 120, height = 1, blend = 60 },
                                { x = M.MINE.x, z = M.MINE.z, radius = 40, height = 1, blend = 30 } } }
  prts.scene.env{
    sun_dir = { 0.35, 0.55, 0.45 }, sun_color = { 1.0, 0.9, 0.78 }, ambient = { 0.34, 0.36, 0.40 },
    fog_color = { 0.66, 0.70, 0.72 }, fog_near = 140, fog_far = 700,
    sky_horizon = { 0.74, 0.78, 0.80 }, sky_zenith = { 0.40, 0.50, 0.66 }, stars = 0, moons = false,
  }
  S.match() -- trees and mines belong to the match registry
  local rand = B.rng(77)
  forest(rand, -60, -150, 80, 25, 40)
  forest(rand, -40, 150, 90, 25, 40)
  M.fort_top = fort()
  for i = 1, 3 do
    Horse.make{ name = "horse." .. i, pos = { M.CAMP.x + 26 + i * 3, prts.scene.height(M.CAMP.x + 26, 24) + 0.0, 24 }, yaw = 0 }
  end
  Wagon.track(Wagon.make{ name = "wagon", road = M.ROAD, start = 1 })
  Tower.make{ name = "siege_tower", pos = { M.TOWER.x, M.TOWER.z }, yaw = -math.pi / 2, stop_at = M.distance_to_wall }
  Mine.make{ name = "gold_mine", pos = { M.MINE.x, M.MINE.z } }
  prts.scene.marker("start", { M.CAMP.x + 30, prts.scene.height(M.CAMP.x + 30, 18), 18 }, 0)
  prts.scene.marker("tower_door", { M.TOWER.x - 5.2, prts.scene.height(M.TOWER.x - 5.2, 0) + 0.1, 0 }, -math.pi / 2)
  prts.scene.marker("adit", { M.MINE.x, prts.scene.height(M.MINE.x, M.MINE.z + 4) + 0.1, M.MINE.z + 4 }, 0)
  return true
end

-- How far the siege tower's front is from the fort's west wall (its outer face).
function M.distance_to_wall(tower)
  local x = prts.entity.pos(tower)
  return M.FORT.x0 - (x + Tower.FRONT)
end

-- The fort's towers shoot at the siege tower while it is within 50 m of the wall: a bolt every
-- two seconds into one of its rooms.
M.ROOMS = { "push", "middle", "top" }
function M.fort_shoots(now)
  if now % 60 ~= 0 then return nil end
  local t = prts.entity.find("siege_tower")
  if not t or prts.entity.data(t).at_wall then return nil end
  local left = M.distance_to_wall(t)
  if left > 50 then return nil end
  local room = M.ROOMS[(now // 60) % #M.ROOMS + 1]
  Tower.damage(t, room, 6)
  return room
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", color = "crown", ai = false, x = M.CAMP.x - 30, z = M.CAMP.z },
  { id = 2, name = "Lord Varric's host", color = "red", ai = false, x = 160, z = 0 },
}

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -500, -300, 500, 300 }
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local Bld = require("rts.structures")
  match.start{ sides = M.SIDES, serfs = 3 }
  Bld.place(1, "crown.stables", M.CAMP.x + 30, M.CAMP.z + 40, true)
  Bld.place(1, "crown.farmstead", M.CAMP.x - 30, M.CAMP.z + 45, true)
  local knights = S.new_company(1, "Knights")
  for i = 1, 4 do S.spawn_unit(1, "crown.knight", M.CAMP.x + 20 + i * 2, M.CAMP.z - 14, knights) end
  local guard = S.new_company(2, "Foot")
  for i = 1, 8 do S.spawn_unit(2, "crown.men_at_arms", 150 + (i % 4) * 1.5, -20 + i // 4 * 1.5, guard) end
  return true
end

prts.events.on("session_started", "maps.greyfen_march", M.on_started)
prts.events.on("tick", "maps.greyfen_march", function(now)
  if require("game.session").current == M.ID then M.fort_shoots(now) end
end)
return M
