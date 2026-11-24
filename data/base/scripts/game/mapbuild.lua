-- Building a map from a map file (game/mapfile.lua), for the editor and for playing it. The
-- editor (mode "edit") shows every object as a stand-in it can move and remove: the terrain,
-- props and ruins as they will be, trees and mines as their shapes, a holdfast as its banner
-- pole and footprint, a company as one of its soldiers. Playing it (mode "play") builds the
-- real things: trees, mines and springs to gather, the sides' holdfasts and companies in a
-- match, and the Imperium ruins held by the Custodians. MIT License.
local Mf = require("game.mapfile")
local S = require("rts.state")

local Mb = {}

Mb.ENV = {
  day = { sun_dir = { 0.4, 0.7, -0.4 }, sun_color = { 1.0, 0.95, 0.85 }, ambient = { 0.38, 0.40, 0.45 },
          fog_color = { 0.75, 0.80, 0.86 }, sky_horizon = { 0.76, 0.82, 0.90 }, sky_zenith = { 0.30, 0.48, 0.78 },
          stars = 0, moons = false },
  dusk = { sun_dir = { 0.8, 0.18, -0.3 }, sun_color = { 1.0, 0.62, 0.36 }, ambient = { 0.30, 0.27, 0.30 },
           fog_color = { 0.62, 0.48, 0.42 }, sky_horizon = { 0.90, 0.55, 0.38 }, sky_zenith = { 0.20, 0.24, 0.42 },
           stars = 0.4, moons = true },
  night = { sun_dir = { 0.2, 0.6, 0.3 }, sun_color = { 0.32, 0.38, 0.58 }, ambient = { 0.14, 0.15, 0.22 },
            fog_color = { 0.08, 0.09, 0.14 }, sky_horizon = { 0.10, 0.12, 0.20 }, sky_zenith = { 0.02, 0.03, 0.08 },
            stars = 1, moons = true },
}

-- Which unit a company of a side's faction is, when the file names another faction's.
Mb.COMPANY = "crown.men_at_arms"

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.80, 0.94, 0.70 }, uv_scale = 4 }
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
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  require("props.horse").materials()
end

-- The terrain, light and sky of a map.
function Mb.ground(map)
  materials()
  prts.scene.cells{ "outside" }
  local flats = {}
  for _, o in ipairs(map.objects) do
    if o.kind == "holdfast" then flats[#flats + 1] = { x = o.x, z = o.z, radius = 60, height = 3, blend = 50 } end
  end
  prts.scene.terrain{ size = map.size, spacing = 4, seed = map.seed, height = map.hills, octaves = 4, material = "grass",
                      flats = flats }
  local env = {}
  for k, v in pairs(Mb.ENV[map.time] or Mb.ENV.day) do env[k] = v end
  env.fog_near, env.fog_far = 160, math.max(600, map.size)
  prts.scene.env(env)
end

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid == true }
end

-- A stand-in for one object in the editor, as an entity named `name`. Returns the entity.
function Mb.stand_in(o, name)
  local y = prts.scene.height(o.x, o.z)
  local e = prts.entity.spawn{ name = name, pos = { o.x, y, o.z }, yaw = o.yaw or 0 }
  if o.kind == "tree" then
    prts.scene.prism{ base = { 0, -0.2, 0 }, radius = 0.35, top_radius = 0.25, height = 3, sides = 5, material = "bark",
                      entity = e }
    prts.scene.prism{ base = { 0, 2.2, 0 }, radius = 2.2, top_radius = 0, height = 4.5, sides = 6, material = "pine",
                      entity = e }
  elseif o.kind == "mine" then
    box(e, "rock", -3, -0.5, -3, 3, 3.5, 3)
  elseif o.kind == "spring" then
    box(e, "aether_glow", -2.2, -0.1, -2.2, 2.2, 0.08, 2.2)
  elseif o.kind == "ruin" then
    for k = 0, 6 do
      local a = k * math.pi * 2 / 7
      box(e, "stone", math.cos(a) * 9 - 0.6, -0.5, math.sin(a) * 9 - 0.6, math.cos(a) * 9 + 0.6, 1.2 + (k * 37 % 5) * 0.5,
          math.sin(a) * 9 + 0.6)
    end
  elseif o.kind == "prop" then
    prts.scene.model{ path = Mf.PROPS[o.id], entity = e, pos = { 0, 0, 0 },
                      scale = Mf.PROPS[o.id]:match("%.vox$") and 0.07 or 1 }
  elseif o.kind == "holdfast" then
    local mat = o.id == "crown" and "banner" or "banner_red"
    box(e, "oak", -0.15, 0, -0.15, 0.15, 8, 0.15)
    box(e, mat, 0.15, 5.5, -0.04, 2.4, 7.8, 0.04)
    for _, s in ipairs({ { -11, -11, 11, -10.8 }, { -11, 10.8, 11, 11 }, { -11, -11, -10.8, 11 }, { 10.8, -11, 11, 11 } }) do
      box(e, "stone", s[1], 0, s[2], s[3], 0.3, s[4])
    end
  elseif o.kind == "company" then
    prts.scene.sprite{ sprite = "sprites/" .. (o.side == 1 and "crown_guard" or "red_guard") .. ".lua", entity = e,
                       anim = "idle" }
    box(e, "iron", -0.05, 0, -0.05, 0.05, 2.6, 0.05)
  end
  return e
end

-- The map built to be played: what the objects are, a match between the holdfasts' sides, and
-- the Custodians at the ruins. Returns the number of sides.
function Mb.play(map)
  local E = require("rts.economy")
  S.match()
  for _, o in ipairs(map.objects) do
    if o.kind == "tree" then E.add_tree(o.x, o.z, 1)
    elseif o.kind == "mine" then E.add_mine(o.x, o.z)
    elseif o.kind == "spring" then E.add_spring(o.x, o.z)
    elseif o.kind == "prop" then
      local y = prts.scene.height(o.x, o.z)
      prts.scene.model{ path = Mf.PROPS[o.id], pos = { o.x, y, o.z }, yaw = o.yaw,
                        scale = Mf.PROPS[o.id]:match("%.vox$") and 0.07 or 1, solid = true }
    end
  end
  return #Mf.sides(map)
end

-- After the map is finished (the navigation grid is built): the match, if the map has at least
-- two sides. Side 1 is the player's; the others are commanded by the AI.
function Mb.match(map)
  local holds = Mf.sides(map)
  if #holds < 2 then return false end
  local sides = {}
  for _, h in ipairs(holds) do
    sides[#sides + 1] = { id = h.side, name = h.side == 1 and "Crown of Esterholt" or ("Side " .. h.side),
                          faction = Mf.HOLDFASTS[h.id], color = h.side == 1 and "crown" or "red", ai = h.side ~= 1,
                          x = h.x, z = h.z, gold = 500, timber = 400, stone = 300, pearls = 50 }
  end
  local ruins = {}
  for _, o in ipairs(map.objects) do
    if o.kind == "ruin" then ruins[#ruins + 1] = { x = o.x, z = o.z } end
  end
  require("rts.match").start{ sides = sides, serfs = 4, custodians = #ruins > 0 and "neutral" or nil, ruins = ruins }
  for _, o in ipairs(map.objects) do
    if o.kind == "company" and S.side(o.side) then
      local def = require("rts.defs").unit(o.id) and o.id or Mb.COMPANY
      local c = S.new_company(o.side, "Foot")
      for i = 1, 6 do S.spawn_unit(o.side, def, o.x - 3 + (i % 3) * 1.4, o.z + (i // 3) * 1.4, c) end
    end
  end
  return true
end

-- The commander's start marker.
function Mb.start(map)
  local x, z = map.start[1], map.start[2]
  prts.scene.marker("start", { x, prts.scene.height(x, z) + 0.1, z }, map.start[3] or 0)
end

return Mb
