-- Structures: placing a site, building it up, training units, and destruction. A structure is
-- an entity at the centre of its footprint whose boxes are its own (they go when it falls);
-- its footprint blocks the navigation grid. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local B = {}
B.DOOR_GAP = 3 -- metres in front of the door where trained units appear

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- The building's shape around its entity (local coordinates; the door faces +z).
local SHAPES = {}

function SHAPES.holdfast(e, w, d, color)
  local hw, hd = w / 2, d / 2
  -- a low curtain with a gate gap on the door side, and the keep in the middle
  box(e, "stone", -hw, 0, -hd, hw, 3, -hd + 0.8)
  box(e, "stone", -hw, 0, -hd, -hw + 0.8, 3, hd)
  box(e, "stone", hw - 0.8, 0, -hd, hw, 3, hd)
  box(e, "stone", -hw, 0, hd - 0.8, -2.5, 3, hd)
  box(e, "stone", 2.5, 0, hd - 0.8, hw, 3, hd)
  -- the keep: walls with a door on the courtyard side, a war room inside, a roof; it grows with
  -- the stage (holdfast, keep, high castle)
  local k = math.max(6, math.floor(w / 4))
  local h = 12 + (k - 6) * 2
  box(e, "stone", -k, 0, -k, k, h, -k + 0.8)
  box(e, "stone", -k, 0, -k, -k + 0.8, h, k)
  box(e, "stone", k - 0.8, 0, -k, k, h, k)
  box(e, "stone", -k, 0, k - 0.8, -1.2, h, k)
  box(e, "stone", 1.2, 0, k - 0.8, k, h, k)
  box(e, "stone", -1.2, 3, k - 0.8, 1.2, h, k)
  box(e, "planks", -k, h - 0.4, -k, k, h, k)
  box(e, "stone_floor", -k + 0.8, 0, -k + 0.8, k - 0.8, 0.05, k - 0.8, false)
  box(e, color, -0.5, h, -0.5, 0.5, h + 4, 0.5, false) -- the banner pole
  if k >= 7 then -- corner towers from the keep stage on
    for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
      local x0, z0 = c[1] * hw - (c[1] > 0 and 3 or 0), c[2] * hd - (c[2] > 0 and 3 or 0)
      box(e, "stone", x0, 0, z0, x0 + 3, 6 + k / 2, z0 + 3)
    end
  end
end

function SHAPES.hall(e, w, d, h, mat)
  local hw, hd = w / 2, d / 2
  box(e, mat, -hw, 0, -hd, hw, h, -hd + 0.4)
  box(e, mat, -hw, 0, -hd, -hw + 0.4, h, hd)
  box(e, mat, hw - 0.4, 0, -hd, hw, h, hd)
  box(e, mat, -hw, 0, hd - 0.4, -1.2, h, hd)
  box(e, mat, 1.2, 0, hd - 0.4, hw, h, hd)
  box(e, mat, -1.2, 2.6, hd - 0.4, 1.2, h, hd)
  prts.scene.wedge{ min = { -hw - 0.3, h, -hd - 0.3 }, max = { 0, h + 2.5, hd + 0.3 }, axis = "x", sign = 1,
                    material = "slate", entity = e }
  prts.scene.wedge{ min = { 0, h, -hd - 0.3 }, max = { hw + 0.3, h + 2.5, hd + 0.3 }, axis = "x", sign = -1,
                    material = "slate", entity = e }
end

function SHAPES.farmstead(e, w, d)
  SHAPES.hall(e, w * 0.6, d * 0.5, 3.5, "plaster")
  box(e, "dirt", -w / 2, 0, d * 0.3, w / 2, 0.15, d / 2, false) -- the field
end

function SHAPES.watchtower(e, w, d)
  local hw = w / 2
  for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
    box(e, "oak", c[1] * hw - 0.3, 0, c[2] * hw - 0.3, c[1] * hw + 0.3, 12, c[2] * hw + 0.3)
  end
  box(e, "planks", -hw - 0.5, 12, -hw - 0.5, hw + 0.5, 12.3, hw + 0.5)
  prts.scene.prism{ base = { 0, 13.5, 0 }, radius = hw + 0.8, top_radius = 0, height = 3, sides = 4, material = "slate", entity = e }
end

-- Stables: a long timber stall row with a paddock fence in front of it.
function SHAPES.stables(e, w, d)
  local hw, hd = w / 2, d / 2
  box(e, "oak", -hw, 0, -hd, hw, 3.2, -hd + 0.4)
  box(e, "oak", -hw, 0, -hd, -hw + 0.4, 3.2, 0)
  box(e, "oak", hw - 0.4, 0, -hd, hw, 3.2, 0)
  for i = 0, 5 do -- the stall partitions
    local x = -hw + 0.4 + i * (w - 0.8) / 5
    box(e, "planks", x - 0.1, 0, -hd + 0.4, x + 0.1, 1.6, -1)
  end
  prts.scene.wedge{ min = { -hw - 0.3, 3.2, -hd - 0.3 }, max = { hw + 0.3, 4.6, 0.3 }, axis = "z", sign = 1,
                    material = "slate", entity = e }
  for i = 0, 8 do -- the paddock fence
    local x = -hw + i * w / 8
    box(e, "oak", x - 0.08, 0, hd - 0.2, x + 0.08, 1.3, hd)
  end
  box(e, "oak", -hw, 1.0, hd - 0.15, -2, 1.12, hd - 0.05, false)
  box(e, "oak", 2, 1.0, hd - 0.15, hw, 1.12, hd - 0.05, false)
end

-- Wildborn timber halls: a long hall of planks and hide with antlers on the gables and a door in
-- the +z end; the war room (the war table's place) is at its middle.
function SHAPES.lodge(e, w, d, stage)
  local hw, hd = w / 2 - 2, d / 2 - 2
  local h = 5 + stage
  box(e, "planks", -hw, 0, -hd, hw, h, -hd + 0.5)
  box(e, "planks", -hw, 0, -hd, -hw + 0.5, h, hd)
  box(e, "planks", hw - 0.5, 0, -hd, hw, h, hd)
  box(e, "planks", -hw, 0, hd - 0.5, -1.3, h, hd)
  box(e, "planks", 1.3, 0, hd - 0.5, hw, h, hd)
  box(e, "planks", -1.3, 3, hd - 0.5, 1.3, h, hd)
  prts.scene.wedge{ min = { -hw - 0.5, h, -hd - 0.5 }, max = { 0, h + 3 + stage, hd + 0.5 }, axis = "x", sign = 1,
                    material = "hide", entity = e }
  prts.scene.wedge{ min = { 0, h, -hd - 0.5 }, max = { hw + 0.5, h + 3 + stage, hd + 0.5 }, axis = "x", sign = -1,
                    material = "hide", entity = e }
  for _, z in ipairs({ -hd, hd }) do -- antlers on the gables
    box(e, "bone", -1.5, h + 2 + stage, z - 0.1, -0.3, h + 4 + stage, z + 0.1, false)
    box(e, "bone", 0.3, h + 2 + stage, z - 0.1, 1.5, h + 4 + stage, z + 0.1, false)
  end
  box(e, "dirt", -hw + 0.5, 0, -hd + 0.5, hw - 0.5, 0.05, hd - 0.5, false)
  if stage >= 3 then -- the antler throne: totems at the corners
    for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
      box(e, "bone", c[1] * (hw + 1.5) - 0.3, 0, c[2] * (hd + 1.5) - 0.3, c[1] * (hw + 1.5) + 0.3, 9, c[2] * (hd + 1.5) + 0.3)
    end
  end
end

function SHAPES.hut(e, x, z, r)
  prts.scene.prism{ base = { x, 0, z }, radius = r, top_radius = r, height = 2.2, sides = 8, material = "planks",
                    entity = e, solid = true }
  prts.scene.prism{ base = { x, 2.2, z }, radius = r + 0.4, top_radius = 0, height = 2.2, sides = 8, material = "hide",
                    entity = e }
end

-- A ring of fence with a hut in a corner (herd pens, the war pit, the beast den).
function SHAPES.pen(e, w, d, hut, pit)
  local hw, hd = w / 2, d / 2
  for i = 0, 7 do
    local t = i / 8
    box(e, "oak", -hw + t * w - 0.1, 0, -hd, -hw + t * w + 0.1, 1.4, -hd + 0.2)
    box(e, "oak", -hw + t * w - 0.1, 0, hd - 0.2, -hw + t * w + 0.1, 1.4, hd)
    box(e, "oak", -hw, 0, -hd + t * d - 0.1, -hw + 0.2, 1.4, -hd + t * d + 0.1)
    box(e, "oak", hw - 0.2, 0, -hd + t * d - 0.1, hw, 1.4, -hd + t * d + 0.1)
  end
  box(e, "oak", -hw, 1.1, -hd, hw, 1.2, -hd + 0.15, false)
  box(e, "oak", -hw, 1.1, hd - 0.15, hw, 1.2, hd, false)
  SHAPES.hut(e, -hw + 3.5, -hd + 3.5, hut or 2.5)
  if pit then box(e, "dirt", -hw + 1, -0.5, -hd + 1, hw - 1, 0.02, hd - 1, false) end
end

-- A tall carved pole with a platform (totem watch).
function SHAPES.totem(e, w)
  local hw = w / 2
  box(e, "oak", -0.6, 0, -0.6, 0.6, 13, 0.6)
  box(e, "planks", -hw, 12, -hw, hw, 12.3, hw)
  box(e, "bone", -1.4, 13, -0.15, 1.4, 14.5, 0.15, false)
end

-- A ring of standing stones (giant's cairn).
function SHAPES.cairn(e, w)
  for i = 0, 9 do
    local a = i / 10 * 2 * math.pi
    local x, z = math.cos(a) * (w / 2 - 1.5), math.sin(a) * (w / 2 - 1.5)
    box(e, "rock", x - 0.8, 0, z - 0.8, x + 0.8, 4 + (i % 3), z + 0.8)
  end
end

-- A well house (stone) or shrine (timber and hide) over its glowing aether spring.
function SHAPES.well(e, w, stone)
  local hw = w / 2 - 1
  local mat = stone and "stone" or "planks"
  box(e, mat, -hw, 0, -hw, hw, 1.0, -hw + 0.4)
  box(e, mat, -hw, 0, hw - 0.4, hw, 1.0, hw)
  box(e, mat, -hw, 0, -hw, -hw + 0.4, 1.0, hw)
  box(e, mat, hw - 0.4, 0, -hw, hw, 1.0, hw)
  box(e, "aether_glow", -hw + 0.5, 0.2, -hw + 0.5, hw - 0.5, 0.6, hw - 0.5, false)
  box(e, "oak", -0.15, 0, -0.15, 0.15, 3.5, 0.15)
  prts.scene.wedge{ min = { -hw - 0.3, 3.2, -hw - 0.3 }, max = { hw + 0.3, 4.4, hw + 0.3 }, axis = "x", sign = 1,
                    material = stone and "slate" or "hide", entity = e }
end

-- A harbor or raft-yard: a timber hall on the shore and a jetty out over the water (+z).
function SHAPES.harbor(e, w, d, stone)
  local hw, hd = w / 2, d / 2
  SHAPES.hall(e, w * 0.6, d * 0.6, 5, stone and "plaster" or "planks")
  box(e, "planks", -2, 0.0, hd - 2, 2, 0.3, hd + 22)
  for i = 0, 5 do
    local z = hd + i * 4
    box(e, "oak", -2.2, -6, z - 0.2, -1.8, 0.3, z + 0.2)
    box(e, "oak", 1.8, -6, z - 0.2, 2.2, 0.3, z + 0.2)
  end
  box(e, "oak", hw - 6, 0, -hd + 2, hw - 1, 3, hd - 2, false) -- a slipway frame
end

local function build_shape(e, def, color)
  local w, d = def.size[1], def.size[2]
  local kind = def.id:match("%.(.+)$")
  local banner = color == "red" and "banner_red" or "banner"
  if def.mobile then return -- the behemoth carries its own beast (props/behemoth.lua)
  elseif def.hq and D.faction_of(def.id) == "crown" then SHAPES.holdfast(e, w, d, banner)
  elseif def.underhall then -- the Gloam's gatehouse; its hall lies under the ground
    SHAPES.hall(e, w * 0.7, d * 0.5, 6, "stone")
    SHAPES.totem(e, 3)
    require("rts.tunnels").hall(e, def.underhall)
  elseif def.shaft then -- a headframe over a deep shaft
    for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
      box(e, "oak", c[1] * 3 - 0.2, 0, c[2] * 3 - 0.2, c[1] * 3 + 0.2, 7, c[2] * 3 + 0.2)
    end
    box(e, "oak", -3.2, 7, -3.2, 3.2, 7.4, 3.2, false)
    require("rts.tunnels").shaft(e, def.shaft)
  elseif def.sails then return -- the Tidehold builds its own rafts (props/tidehold.lua)
  elseif kind == "shipwright_yard" or kind == "great_dock" then SHAPES.harbor(e, w, d, kind == "great_dock")
  elseif kind == "storm_tower" or kind == "signal_mast" then SHAPES.watchtower(e, math.min(w, 6), math.min(d, 6))
  elseif kind == "harpoon_battery" then SHAPES.watchtower(e, w * 0.6, d * 0.6)
  elseif D.faction_of(def.id) == "brine" and def.hq then SHAPES.holdfast(e, w, d, "banner_blue")
  elseif kind == "echo_pillar" then SHAPES.totem(e, w)
  elseif kind == "glow_tower" then SHAPES.watchtower(e, w, d)
  elseif kind == "glowstone_works" then SHAPES.well(e, w, true)
  elseif D.faction_of(def.id) == "gloam" then SHAPES.hall(e, w, d, 4, "stone")
  elseif def.hq then SHAPES.lodge(e, w, d, def.hq)
  elseif kind == "herd_pen" then SHAPES.pen(e, w, d, 2.5)
  elseif kind == "war_pit" then SHAPES.pen(e, w, d, 3, true)
  elseif kind == "beast_den" then SHAPES.pen(e, w, d, 3.5)
  elseif kind == "timber_camp" or kind == "bone_forge" then SHAPES.hall(e, w * 0.7, d * 0.7, 3.5, "planks")
  elseif kind == "totem_watch" then SHAPES.totem(e, w)
  elseif kind == "giants_cairn" then SHAPES.cairn(e, w)
  elseif kind == "aether_shrine" then SHAPES.well(e, w, false)
  elseif kind == "aether_well" then SHAPES.well(e, w, true)
  elseif kind == "forgeworks" then SHAPES.hall(e, w, d, 6, "stone")
  elseif kind == "harbor" or kind == "raft_yard" then SHAPES.harbor(e, w, d, kind == "harbor")
  elseif kind == "balloon" then require("props.balloon").build(e)
  elseif kind == "roost_cliffs" then SHAPES.cairn(e, w) SHAPES.totem(e, 4)
  elseif kind == "arcanum" then SHAPES.watchtower(e, w * 0.6, d * 0.6) SHAPES.well(e, 4, true)
  elseif kind == "spirit_lodge" then SHAPES.lodge(e, w * 0.8, d * 0.8, 1) SHAPES.totem(e, 3)
  elseif kind == "sky_mast" then SHAPES.watchtower(e, 4, 4) SHAPES.totem(e, 2)
  elseif kind == "smithy" then SHAPES.hall(e, w, d, 4, "planks")
  elseif kind == "farmstead" then SHAPES.farmstead(e, w, d)
  elseif kind == "watchtower" then SHAPES.watchtower(e, w, d)
  elseif kind == "sawmill" then SHAPES.hall(e, w, d, 4, "planks")
  elseif kind == "stables" then SHAPES.stables(e, w, d)
  else SHAPES.hall(e, w, d, 5, "plaster") end
end

-- The look of a site at each stage of building (a child entity, replaced as the stage changes):
-- 1 staked out, 2 foundations, 3 frame and scaffolding, 4 walls and roof. Nothing on a site is
-- solid, so it can be walked through.
B.STAGES = { "staked out", "foundations", "frame", "walls and roof", "finished" }

local function site_look(e, def, stage)
  local name = prts.entity.name(e) .. ".site"
  local old = prts.entity.find(name)
  if old then prts.entity.despawn(old) end
  local v = prts.entity.spawn{ name = name, parent = e, pos = { 0, 0, 0 } }
  local hw, hd = def.size[1] / 2, def.size[2] / 2
  local function b(mat, x0, y0, z0, x1, y1, z1) box(v, mat, x0, y0, z0, x1, y1, z1, false) end
  for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do -- the stakes
    b("oak", c[1] * hw - 0.08, 0, c[2] * hd - 0.08, c[1] * hw + 0.08, 0.9, c[2] * hd + 0.08)
  end
  b("dirt", -hw, 0, -hd, hw, 0.06, hd)
  if stage >= 2 then b("stone_floor", -hw + 0.3, 0, -hd + 0.3, hw - 0.3, 0.45, hd - 0.3) end
  if stage >= 3 then
    local h = stage >= 4 and 4.5 or 3.0
    for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }, { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } }) do
      b("oak", c[1] * (hw - 0.4) - 0.15, 0.45, c[2] * (hd - 0.4) - 0.15, c[1] * (hw - 0.4) + 0.15, h, c[2] * (hd - 0.4) + 0.15)
    end
    b("planks", -hw, 2.2, -hd, hw, 2.3, -hd + 0.6) -- the scaffold walks
    b("planks", -hw, 2.2, hd - 0.6, hw, 2.3, hd)
  end
  if stage >= 4 then
    local mat = D.burns(def) and "planks" or "stone"
    b(mat, -hw + 0.3, 0.45, -hd + 0.3, hw - 0.3, 2.6, -hd + 0.6)
    b(mat, -hw + 0.3, 0.45, hd - 0.6, -1.2, 2.6, hd - 0.3)
    b(mat, 1.2, 0.45, hd - 0.6, hw - 0.3, 2.6, hd - 0.3)
    b(mat, -hw + 0.3, 0.45, -hd + 0.3, -hw + 0.6, 2.6, hd - 0.3)
    b(mat, hw - 0.6, 0.45, -hd + 0.3, hw - 0.3, 2.6, hd - 0.3)
    b("slate", -hw, 4.4, -hd, hw, 4.5, hd)
  end
  return v
end

-- The materials every skirmish map defines for the buildings of both factions (and fire).
function B.materials()
  local m = prts.scene.material
  m{ id = "hide", texture = "textures/plaster.png", tint = { 0.72, 0.56, 0.38 }, uv_scale = 2 }
  m{ id = "bone", texture = "textures/plaster.png", tint = { 1.05, 1.0, 0.86 }, uv_scale = 1 }
  m{ id = "flame", texture = "textures/flame.png", uv_scale = 1, emissive = true }
  m{ id = "aether_glow", texture = "textures/iron.png", tint = { 0.5, 1.2, 1.6 }, uv_scale = 1, emissive = true }
  m{ id = "scorched", texture = "textures/iron.png", tint = { 0.16, 0.14, 0.13 }, uv_scale = 1 }
  m{ id = "coral", texture = "textures/plaster.png", tint = { 1.3, 0.62, 0.55 }, uv_scale = 1 }
  m{ id = "banner_blue", texture = "textures/banner_crown.png", tint = { 0.35, 0.9, 1.3 }, uv_scale = 1.6 }
  m{ id = "sail_blue", texture = "textures/plaster.png", tint = { 0.75, 0.95, 1.25 }, uv_scale = 2 }
end

-- The footprint rectangle { x0, z0, x1, z1 } of def centred on (x, z).
function B.footprint(def, x, z)
  local hw, hd = def.size[1] / 2, def.size[2] / 2
  return { x - hw, z - hd, x + hw, z + hd }
end

-- Whether def fits at (x, z): every grid cell under it (with a 2 m margin) walkable and free.
function B.can_place(def_id, x, z)
  local def = D.structure(def_id)
  local fp = B.footprint(def, x, z)
  local step = prts.nav.info().cell
  for px = fp[1] - 2, fp[3] + 2, step do
    for pz = fp[2] - 2, fp[4] + 2, step do
      if not prts.nav.walkable(px, pz) then return false end
    end
  end
  return true
end

-- The free aether spring within reach of (x, z), for wells and shrines (kind "grotto": a
-- glowstone grotto's vent, for glowstone works).
function B.spring_at(x, z, kind)
  kind = kind or "spring"
  local reach = kind == "grotto" and D.GROTTO_REACH or D.SPRING_REACH
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      if d.kind == kind and not (d.well and S.alive(d.well)) then
        local sx, _, sz = S.pos(e)
        if S.dist(x, z, sx, sz) <= reach then return e end
      end
    end
  end
  return nil
end

-- Whether deep water (over 2 m) lies within D.SHORE of (x, z): a League hall's shore.
function B.shore_at(x, z)
  local Wa = require("rts.water")
  for r = 6, D.SHORE, 6 do
    for i = 0, 11 do
      local a = i / 12 * 2 * math.pi
      if Wa.depth(x + math.cos(a) * r, z + math.sin(a) * r) > 2 then return true end
    end
  end
  return false
end

-- Places a structure for side at (x, z). `built` makes it finished at once (starting
-- headquarters, map set pieces); otherwise it is a site the side pays for and workers build.
-- Returns the entity or nil, reason.
function B.place(side_id, def_id, x, z, built)
  local def = D.structure(def_id)
  local side = S.side(side_id)
  if not def then return nil, "unknown" end
  local spring = (def.aether and B.spring_at(x, z)) or (def.glowstone and B.spring_at(x, z, "grotto"))
  if not built then
    if def.mobile then return nil, "not built in place" end
    if not require("rts.tiers").allowed(side_id, def) then return nil, "needs tier " .. def.tier end
    if def.aether and not spring then return nil, "needs an aether spring" end
    if def.glowstone and not spring then return nil, "needs a glowstone grotto" end
    if def.shore and not B.shore_at(x, z) then return nil, "needs a shore" end
    if def.harbor and not require("rts.fleet").shore_ok(def, x, z) then return nil, "needs deep water" end
    if not B.can_place(def_id, x, z) then return nil, "no room" end
    if not S.pay(side, def.cost) then return nil, "cannot pay" end
  end
  local m = S.match()
  local y = prts.nav.height(x, z) or prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = string.format("structure.%d.%d", side_id, #m.structures + 1), pos = { x, y, z } }
  local d = prts.entity.data(e)
  d.kind, d.side, d.def = "structure", side_id, def_id
  d.fp = B.footprint(def, x, z)
  d.built = built == true
  d.progress = built and def.build or 0
  d.hp = built and def.hp or math.floor(def.hp * 0.1)
  d.queue, d.train, d.acc = {}, 0, 0
  d.rally = { x, z + def.size[2] / 2 + B.DOOR_GAP + 4 }
  d.cd = 0
  d.stage = built and 5 or 1
  if spring then
    d.spring = spring
    S.data(spring).well = e
  end
  if not def.mobile then prts.nav.block{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } } end
  if d.built then build_shape(e, def, side.color) else site_look(e, def, 1) end
  m.structures[#m.structures + 1] = e
  prts.events.emit("rts_structure_placed", e, side_id, def_id)
  return e
end

-- Re-creates a structure's entity (its look changes completely: finished, a new stage of
-- headquarters). The data moves to the new entity; returns it.
function B.rebuild(e, def_id)
  local d = S.data(e)
  local x, y, z = S.pos(e)
  local m = S.match()
  local fresh = prts.entity.spawn{ name = prts.entity.name(e) .. "b", pos = { x, y, z } }
  local fd = prts.entity.data(fresh)
  for k, v in pairs(d) do fd[k] = v end
  fd.def = def_id or d.def
  fd.furnished = nil
  for i, s in ipairs(m.structures) do
    if s == e then m.structures[i] = fresh end
  end
  if fd.spring and S.alive(fd.spring) then S.data(fd.spring).well = fresh end
  S.release_commander(e)
  prts.entity.despawn(e)
  build_shape(fresh, D.structure(fd.def), S.side(fd.side).color)
  return fresh
end

-- Work on a site by one worker (`by`) for `seconds`; the commander's presence speeds it by a
-- quarter. Returns true and the finished entity when the work is done.
function B.work(e, seconds, by)
  local d = S.data(e)
  if d.built then return true end
  local def = D.structure(d.def)
  local x, _, z = S.pos(by or e)
  if require("rts.presence").near(d.side, x, z) then seconds = seconds * 1.25 end
  d.progress = math.min(def.build, d.progress + seconds)
  d.hp = math.max(d.hp, math.floor(def.hp * (0.1 + 0.9 * d.progress / def.build)))
  local stage = math.min(4, 1 + math.floor(d.progress / def.build * 4))
  if stage ~= d.stage and d.progress < def.build then
    d.stage = stage
    site_look(e, def, stage)
    prts.events.emit("rts_site_stage", e, d.side, stage)
  end
  if d.progress >= def.build then
    d.built, d.hp, d.stage = true, def.hp, 5
    local fresh = B.rebuild(e)
    prts.events.emit("rts_structure_built", fresh, d.side, d.def)
    S.dispatch(d.side, def.name, "The " .. def.name:lower() .. " is finished.")
    return true, fresh
  end
  return false
end

-- What an order at a structure is: "unit", "upgrade", "hq_up" (the next stage of headquarters)
-- or "mobile" (a structure that walks out: the behemoth), its definition and its time.
function B.order_kind(item, structure_def)
  if item == "hq_up" then
    local next = structure_def.next and D.structure(structure_def.next)
    return "hq_up", next, next and next.build
  end
  local u = D.unit(item)
  if u then return "unit", u, u.train end
  local up = D.upgrade(item)
  if up then return "upgrade", up, up.time end
  local st = D.structure(item)
  if st and st.mobile then return "mobile", st, st.build end
  local ship = D.ship(item)
  if ship then return "ship", ship, ship.build end
  local air = D.airship(item)
  if air then return "airship", air, air.build end
  return nil
end

-- Orders something at a finished structure and pays for it at once: a unit, an upgrade, the
-- next headquarters stage ("hq_up") or a behemoth. Returns true, or false and why.
function B.order(e, item)
  local d = S.data(e)
  local def = D.structure(d.def)
  local side = S.side(d.side)
  local Ti = require("rts.tiers")
  if not d.built then return false, "not built" end
  if #d.queue >= 5 then return false, "the queue is full" end
  local kind, it = B.order_kind(item, def)
  if not kind or not it then return false, "unknown order" end
  if kind == "hq_up" then
    for _, q in ipairs(d.queue) do
      if q == "hq_up" then return false, "already rising" end
    end
  elseif kind == "upgrade" then
    if it.at ~= d.def then return false, "not researched here" end
    if Ti.upgrade_known(d.side, item) then return false, "already known" end
  elseif kind == "unit" then
    if not D.trains(it, d.def) then return false, "not trained here" end
  elseif kind == "ship" or kind == "airship" then
    if it.at ~= d.def then return false, "not built here" end
  elseif it.at ~= d.def then
    return false, "not raised here"
  end
  if not Ti.allowed(d.side, it) then return false, "needs tier " .. it.tier end
  if kind == "unit" and S.folk(d.side) + it.folk > S.food(d.side) then
    S.dispatch(d.side, "Quartermaster", "We need more food before we can raise more " .. it.name:lower() .. ".")
    return false, "no food"
  end
  if not S.pay(side, it.cost) then return false, "cannot pay" end
  d.queue[#d.queue + 1] = item
  return true
end

B.order_unit = B.order

-- One second-tick of a structure's queue (called every 30 ticks).
function B.train_tick(e, seconds)
  local d = S.data(e)
  if not d.built or #d.queue == 0 then return nil end
  local def = D.structure(d.def)
  local kind, it, time = B.order_kind(d.queue[1], def)
  d.train = d.train + seconds
  if d.train < time then return nil end
  d.train = 0
  local item = table.remove(d.queue, 1)
  if kind == "unit" then return B.muster(e, item) end
  if kind == "upgrade" then
    S.side(d.side).upgrades[item] = true
    S.dispatch(d.side, def.name, it.name .. ": the work is done.")
    prts.events.emit("rts_upgraded", d.side, item)
    return item
  end
  if kind == "hq_up" then return B.raise_stage(e) end
  local x, _, z = S.pos(e)
  if kind == "airship" then -- it lifts off over the mast
    local Sh = require("props.skyship")
    local n = 1
    while prts.entity.find("sky." .. d.side .. "." .. n) do n = n + 1 end
    local ship = Sh.spawn("sky." .. d.side .. "." .. n, x, z + def.size[2] / 2 + 14, 0, it.kind, d.side)
    S.dispatch(d.side, it.name, "Cast off and aloft.")
    prts.events.emit("rts_airship", ship, d.side, item)
    return ship
  end
  if kind == "ship" then
    local sea = d.seaward or 1 -- which side of the building the water is on (+z, or -z)
    return require("rts.fleet").launch(d.side, item, x, z + sea * (def.size[2] / 2 + 18), sea > 0 and math.pi or 0)
  end
  if it.wyrm then return require("props.wyrm").spawn(d.side, x, z + def.size[2] / 2 + 20) end
  if it.sails then -- the Tidehold is floated out off the dock's seaward side
    local sea = d.seaward or 1
    return require("props.tidehold").spawn(d.side, x, z + sea * (def.size[2] / 2 + 40))
  end
  return require("props.behemoth").spawn(d.side, item, x, z + def.size[2] / 2 + 24)
end

-- A trained squad steps out of the door, as a company if it is one, and goes to the rally point.
function B.muster(e, id)
  local d = S.data(e)
  local u = D.unit(id)
  local x, _, z = S.pos(e)
  local door = z + D.structure(d.def).size[2] / 2 + B.DOOR_GAP
  local company = u.company and S.new_company(d.side, u.company) or nil
  local first
  for i = 1, u.squad do
    local col, row = (i - 1) % 4, math.floor((i - 1) / 4)
    local unit = S.spawn_unit(d.side, id, x - 2.1 + col * 1.4, door + row * 1.4, company)
    first = first or unit
  end
  if company then
    S.dispatch(d.side, company.name, company.name .. ", ready for orders.")
    require("rts.companies").order(company, { kind = "move", x = d.rally[1], z = d.rally[2] })
  end
  prts.events.emit("rts_trained", first, d.side, id)
  return first
end

-- The headquarters rises to its next stage: the building is rebuilt bigger, its health kept in
-- proportion, its footprint grown on the grid.
function B.raise_stage(e)
  local d = S.data(e)
  local old = D.structure(d.def)
  local new = D.structure(old.next)
  prts.nav.unblock{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } }
  local x, _, z = S.pos(e)
  d.hp = math.floor(new.hp * d.hp / old.hp)
  d.fp = B.footprint(new, x, z)
  d.rally = { x, z + new.size[2] / 2 + B.DOOR_GAP + 4 }
  prts.nav.block{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } }
  local fresh = B.rebuild(e, new.id)
  S.dispatch(d.side, new.name, "The " .. old.name:lower() .. " is now a " .. new.name:lower() .. ".")
  prts.events.emit("rts_structure_built", fresh, d.side, new.id)
  prts.events.emit("rts_hq_stage", fresh, d.side, new.hq)
  return fresh
end

-- `seconds` of a structure's own work: aether from a well or shrine, powder at the Forgeworks,
-- bone from a herd pen. Returns what it made, if anything.
function B.produce(e, seconds)
  local d = S.data(e)
  if not d.built then return nil end
  local def = D.structure(d.def)
  local side = S.side(d.side)
  d.acc = (d.acc or 0) + seconds
  local made
  if def.aether then
    while d.acc >= D.AETHER_EVERY do
      d.acc = d.acc - D.AETHER_EVERY
      side.aether = side.aether + 1
      made = "aether"
    end
  elseif def.powder then
    local P = D.POWDER
    if d.acc >= P.every then
      d.acc = d.acc - P.every
      if side.powder < P.max and S.pay(side, { gold = P.gold, timber = P.timber }) then
        side.powder = side.powder + P.powder
        made = "powder"
      end
    end
  elseif def.glowstone then -- glowstone (and aether from the seep) from a works over a grotto
    if not d.spring then d.acc = 0 return nil end
    while d.acc >= D.GLOWSTONE_EVERY do
      d.acc = d.acc - D.GLOWSTONE_EVERY
      side.glowstone = (side.glowstone or 0) + 1
      d.seep = (d.seep or 0) + 1
      if d.seep % 2 == 0 then side.aether = side.aether + 1 end
      made = "glowstone"
    end
  elseif def.bone then
    while d.acc >= D.BONE_EVERY do
      d.acc = d.acc - D.BONE_EVERY
      side.bone = side.bone + 1
      made = "bone"
    end
  else
    d.acc = 0
  end
  return made
end

-- Damage to a structure; it falls at 0 hp. Returns true if it fell.
function B.damage(e, amount)
  local d = S.data(e)
  d.hp = d.hp - amount
  if d.hp > 0 then return false end
  local def = D.structure(d.def)
  if not def.mobile then prts.nav.unblock{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } } end
  if d.spring and S.alive(d.spring) then S.data(d.spring).well = nil end
  local x, y, z = S.pos(e)
  if prts.audio then prts.audio.play{ sound = "sounds/portcullis_slam.wav", at = { x, y + 2, z }, range = 300 } end
  S.dispatch(d.side, def.name, "The " .. def.name:lower() .. " has fallen.")
  prts.events.emit("rts_structure_fell", e, d.side, d.def)
  S.release_commander(e)
  prts.entity.despawn(e)
  return true
end

return B
