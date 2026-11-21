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
  -- the keep: walls with a door on the courtyard side, a war room inside, a roof
  local k = 6
  box(e, "stone", -k, 0, -k, k, 12, -k + 0.8)
  box(e, "stone", -k, 0, -k, -k + 0.8, 12, k)
  box(e, "stone", k - 0.8, 0, -k, k, 12, k)
  box(e, "stone", -k, 0, k - 0.8, -1.2, 12, k)
  box(e, "stone", 1.2, 0, k - 0.8, k, 12, k)
  box(e, "stone", -1.2, 3, k - 0.8, 1.2, 12, k)
  box(e, "planks", -k, 11.6, -k, k, 12, k)
  box(e, "stone_floor", -k + 0.8, 0, -k + 0.8, k - 0.8, 0.05, k - 0.8, false)
  box(e, color, -0.5, 12, -0.5, 0.5, 16, 0.5, false) -- the banner pole
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

-- The scaffolding of a site: a frame of poles that grows with the work.
local function scaffold(e, w, d, frac)
  local hw, hd, h = w / 2, d / 2, 1 + 5 * frac
  for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
    box(e, "oak", c[1] * hw - 0.15, 0, c[2] * hd - 0.15, c[1] * hw + 0.15, h, c[2] * hd + 0.15, false)
  end
  box(e, "dirt", -hw, 0, -hd, hw, 0.1, hd, false)
end

local function build_shape(e, def, color)
  local w, d = def.size[1], def.size[2]
  local kind = def.id:match("%.(.+)$")
  if kind == "holdfast" then SHAPES.holdfast(e, w, d, color == "red" and "banner_red" or "banner")
  elseif kind == "farmstead" then SHAPES.farmstead(e, w, d)
  elseif kind == "watchtower" then SHAPES.watchtower(e, w, d)
  elseif kind == "sawmill" then SHAPES.hall(e, w, d, 4, "planks")
  elseif kind == "stables" then SHAPES.stables(e, w, d)
  else SHAPES.hall(e, w, d, 5, "plaster") end
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

-- Places a structure for side at (x, z). `built` makes it finished at once (starting holdfasts);
-- otherwise it is a site the side pays for and serfs build. Returns the entity or nil, reason.
function B.place(side_id, def_id, x, z, built)
  local def = D.structure(def_id)
  local side = S.side(side_id)
  if not built and not B.can_place(def_id, x, z) then return nil, "no room" end
  if not built and not S.pay(side, def.cost) then return nil, "cannot pay" end
  local m = S.match()
  local y = prts.nav.height(x, z) or prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = string.format("structure.%d.%d", side_id, #m.structures + 1), pos = { x, y, z } }
  local d = prts.entity.data(e)
  d.kind, d.side, d.def = "structure", side_id, def_id
  d.fp = B.footprint(def, x, z)
  d.built = built == true
  d.progress = built and def.build or 0
  d.hp = built and def.hp or math.floor(def.hp * 0.1)
  d.queue, d.train = {}, 0
  d.rally = { x, z + def.size[2] / 2 + B.DOOR_GAP + 4 }
  d.cd = 0
  prts.nav.block{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } }
  if d.built then build_shape(e, def, side.color) else scaffold(e, def.size[1], def.size[2], 0) end
  m.structures[#m.structures + 1] = e
  prts.events.emit("rts_structure_placed", e, side_id, def_id)
  return e
end

-- Work on a site by one serf for `seconds`. Finishes it when the work is done.
function B.work(e, seconds)
  local d = S.data(e)
  if d.built then return true end
  local def = D.structure(d.def)
  d.progress = math.min(def.build, d.progress + seconds)
  d.hp = math.max(d.hp, math.floor(def.hp * (0.1 + 0.9 * d.progress / def.build)))
  if d.progress >= def.build then
    d.built = true
    d.hp = def.hp
    -- the scaffolding goes, the building stands
    local x, y, z = S.pos(e)
    local m = S.match()
    local fresh = prts.entity.spawn{ name = prts.entity.name(e) .. "b", pos = { x, y, z } }
    local fd = prts.entity.data(fresh)
    for k, v in pairs(d) do fd[k] = v end
    for i, s in ipairs(m.structures) do
      if s == e then m.structures[i] = fresh end
    end
    prts.entity.despawn(e)
    build_shape(fresh, def, S.side(fd.side).color)
    prts.events.emit("rts_structure_built", fresh, fd.side, fd.def)
    S.dispatch(fd.side, def.name, "The " .. def.name:lower() .. " is finished.")
    return true, fresh
  end
  return false
end

-- Orders a unit at a finished structure. Returns true, or false and why.
function B.order_unit(e, unit_id)
  local d = S.data(e)
  local u = D.unit(unit_id)
  local side = S.side(d.side)
  if not d.built then return false, "not built" end
  if u.at ~= d.def then return false, "not trained here" end
  if #d.queue >= 5 then return false, "the queue is full" end
  if S.folk(d.side) + u.folk > S.food(d.side) then
    S.dispatch(d.side, "Quartermaster", "We need more food before we can raise more " .. u.name:lower() .. ".")
    return false, "no food"
  end
  if not S.pay(side, u.cost) then return false, "cannot pay" end
  d.queue[#d.queue + 1] = unit_id
  return true
end

-- One second-tick of training at a structure (called every 30 ticks).
function B.train_tick(e, seconds)
  local d = S.data(e)
  if not d.built or #d.queue == 0 then return nil end
  local u = D.unit(d.queue[1])
  d.train = d.train + seconds
  if d.train < u.train then return nil end
  d.train = 0
  local id = table.remove(d.queue, 1)
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

-- Damage to a structure; it falls at 0 hp. Returns true if it fell.
function B.damage(e, amount)
  local d = S.data(e)
  d.hp = d.hp - amount
  if d.hp > 0 then return false end
  local def = D.structure(d.def)
  prts.nav.unblock{ min = { d.fp[1], d.fp[2] }, max = { d.fp[3], d.fp[4] } }
  local x, y, z = S.pos(e)
  if prts.audio then prts.audio.play{ sound = "sounds/portcullis_slam.wav", at = { x, y + 2, z }, range = 300 } end
  S.dispatch(d.side, def.name, "The " .. def.name:lower() .. " has fallen.")
  prts.events.emit("rts_structure_fell", e, d.side, d.def)
  prts.entity.despawn(e)
  return true
end

return B
