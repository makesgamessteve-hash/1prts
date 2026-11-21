-- Castle building blocks: curtain walls with wall-walks and battlements, round towers you can
-- walk into, gatehouses with portcullises, stairs up the walls and gable-roofed halls.
-- Everything is measured in meters; geometry and collision come from lib/build.lua and the
-- scene's tube shape. MIT License.
local B = require("lib.build")
local Portcullis = require("props.portcullis")
local C = {}

C.PARAPET = 1.0      -- height of the solid parapet on a wall-walk's outer edge
C.MERLON = 1.1       -- height of the merlons on top of it
C.MERLON_WIDTH = 1.0
C.CRENEL = 0.8       -- the gap between merlons
C.PARAPET_THICK = 0.6

-- A parapet with merlons along `axis` ("x" or "z") from a0 to a1, across t0..t1, standing on
-- height y. Returns the number of merlons.
function C.battlements(material, axis, a0, a1, t0, t1, y)
  local function box(l0, l1, y0, y1, opts)
    if axis == "x" then B.box(material, l0, y0, t0, l1, y1, t1, opts) else B.box(material, t0, y0, l0, t1, y1, l1, opts) end
  end
  box(a0, a1, y, y + C.PARAPET)
  local step = C.MERLON_WIDTH + C.CRENEL
  local n = math.max(1, math.floor((a1 - a0 + C.CRENEL) / step))
  local start = a0 + ((a1 - a0) - (n * step - C.CRENEL)) / 2
  for i = 0, n - 1 do
    local m0 = start + i * step
    box(m0, m0 + C.MERLON_WIDTH, y + C.PARAPET, y + C.PARAPET + C.MERLON)
  end
  return n
end

-- A curtain wall along `axis` from a0 to a1. `inner` and `outer` are the coordinates of its two
-- faces across the wall (which one is larger tells where the outside is). Its top at `height`
-- is the wall-walk, with battlements on the outer edge. opts: base (bottom, default -1),
-- openings (as for B.wall, e.g. a gate passage). Returns the wall-walk height.
function C.curtain(material, axis, a0, a1, inner, outer, height, opts)
  local o = opts or {}
  local lo, hi = math.min(inner, outer), math.max(inner, outer)
  B.wall(material, axis, a0, a1, lo, hi, o.base or -1, height, o.openings)
  local p0 = outer > inner and hi - C.PARAPET_THICK or lo
  C.battlements(material, axis, a0, a1, p0, p0 + C.PARAPET_THICK, height)
  return height
end

-- The bounding box of a piece of a tower wall: between the fractions s0 and s1 of side i.
local function wall_piece(t, i, s0, s1, y0, y1)
  local step = 2 * math.pi / t.sides
  local a0, a1 = (i - 0.5) * step, (i + 0.5) * step
  local function at(r, s)
    local x0, z0 = math.cos(a0) * r, math.sin(a0) * r
    local x1, z1 = math.cos(a1) * r, math.sin(a1) * r
    return t.x + x0 + (x1 - x0) * s, t.z + z0 + (z1 - z0) * s
  end
  local xs, zs = {}, {}
  for _, p in ipairs({ { t.radius, s0 }, { t.radius, s1 }, { t.inner, s0 }, { t.inner, s1 } }) do
    local x, z = at(p[1], p[2])
    xs[#xs + 1], zs[#zs + 1] = x, z
  end
  B.box(t.material, math.min(table.unpack(xs)), y0, math.min(table.unpack(zs)), math.max(table.unpack(xs)), y1,
        math.max(table.unpack(zs)), { visible = false })
end

-- The side of a tower (with `sides` sides) that faces the direction (dx, dz).
function C.side_towards(sides, dx, dz)
  local a = math.atan(dz, dx)
  return math.floor(a / (2 * math.pi / sides) + 0.5) % sides
end

-- A floor inside a round tower at height y (its top), leaving a square hole of half-size
-- `hole` around the centre (0: no hole).
local function tower_floor(t, y, hole)
  local r = t.inner
  local q = r * 0.72          -- the square inside the circle
  local e = r * 0.97          -- how far the four extensions reach
  local w = r * 0.66
  local y0 = y - 0.3
  local x, z, m = t.x, t.z, t.floor_material or t.material
  if hole and hole > 0 then
    B.box(m, x - q, y0, z - q, x + q, y, z - hole)
    B.box(m, x - q, y0, z + hole, x + q, y, z + q)
    B.box(m, x - q, y0, z - hole, x - hole, y, z + hole)
    B.box(m, x + hole, y0, z - hole, x + q, y, z + hole)
  else
    B.box(m, x - q, y0, z - q, x + q, y, z + q)
  end
  B.box(m, x - w, y0, z + q, x + w, y, z + e)
  B.box(m, x - w, y0, z - e, x + w, y, z - q)
  B.box(m, x + q, y0, z - w, x + e, y, z + w)
  B.box(m, x - e, y0, z - w, x - q, y, z + w)
end

-- A round tower you can walk into. t = { x, z, radius, wall (thickness, default 1.5),
-- height, base (default -1), sides (default 16), material, floor_material, roof_material,
-- floors = { y, ... } (walk heights with a floor; the ground floor needs none),
-- doors = { { side = i, y = walk height, height = 2.6 }, ... },
-- stair = { radius, size, rise, start } (a spiral stair from the ground to the top, through a
-- hatch in the roof floor), roof = "battlements" (default) or "cone", light = colour or false }.
-- Returns the walk height of the roof and, with a stair, the height of its top step.
function C.round_tower(t)
  t.sides = t.sides or 16
  t.wall = t.wall or 1.5
  t.inner = t.radius - t.wall
  t.base = t.base or -1
  local top = t.height
  -- vertical bands: the tower wall is open on a door's side over the door's height
  local cuts = { t.base, top }
  for _, d in ipairs(t.doors or {}) do
    d.height = d.height or 2.6
    cuts[#cuts + 1] = d.y
    cuts[#cuts + 1] = d.y + d.height
  end
  table.sort(cuts)
  local bands = 0
  for k = 1, #cuts - 1 do
    local y0, y1 = cuts[k], cuts[k + 1]
    if y1 - y0 > 0.01 then
      local open = {}
      for _, d in ipairs(t.doors or {}) do
        if d.y <= y0 + 0.01 and d.y + d.height >= y1 - 0.01 then open[d.side] = true end
      end
      local list = {}
      for side in pairs(open) do list[#list + 1] = side end
      table.sort(list)
      prts.scene.tube{ base = { t.x, y0, t.z }, radius = t.radius, inner = t.inner, height = y1 - y0, sides = t.sides,
                       open = list, cap = y1 >= top - 0.01, material = t.material, tile = 3 }
      for i = 0, t.sides - 1 do
        if not open[i] then
          wall_piece(t, i, 0, 0.5, y0, y1)
          wall_piece(t, i, 0.5, 1, y0, y1)
        end
      end
      bands = bands + 1
    end
  end
  -- floors, the stair and the roof
  local hole = 0
  local stair_top
  if t.stair then
    local s = t.stair
    local rise = s.rise or 0.3
    local steps = math.floor((top - 0.0) / rise + 0.5)
    stair_top = B.spiral(t.floor_material or t.material, { center = { t.x, t.z }, base = 0, steps = steps, rise = rise,
                                                           radius = s.radius, size = s.size, start = s.start or 0,
                                                           post = s.post, headroom = 1.0 })
    hole = s.radius + s.size / 2 + 0.15
  end
  if t.ground_floor ~= false then tower_floor(t, 0.02, hole) end
  for _, y in ipairs(t.floors or {}) do tower_floor(t, y, hole) end -- with a stair: landings around it
  tower_floor(t, top, hole)
  if t.roof == "cone" then
    prts.scene.prism{ base = { t.x, top, t.z }, radius = t.radius + 0.4, top_radius = 0, height = t.radius * 1.4,
                      sides = t.sides, material = t.roof_material or "slate" }
  else
    -- a parapet ring with merlons on every other side
    prts.scene.tube{ base = { t.x, top, t.z }, radius = t.radius, inner = t.radius - C.PARAPET_THICK,
                     height = C.PARAPET, sides = t.sides, cap = true, material = t.material }
    local gaps = {}
    for i = 1, t.sides - 1, 2 do gaps[#gaps + 1] = i end
    prts.scene.tube{ base = { t.x, top + C.PARAPET, t.z }, radius = t.radius, inner = t.radius - C.PARAPET_THICK,
                     height = C.MERLON, sides = t.sides, open = gaps, cap = true, material = t.material }
    local ring = { x = t.x, z = t.z, radius = t.radius, inner = t.radius - C.PARAPET_THICK, sides = t.sides,
                   material = t.material }
    for i = 0, t.sides - 1 do wall_piece(ring, i, 0, 1, top, top + C.PARAPET + (i % 2 == 0 and C.MERLON or 0)) end
  end
  if t.light ~= false then
    local c = t.light or { 1.1, 0.7, 0.4 }
    local levels = { 0.2 }
    for _, y in ipairs(t.floors or {}) do levels[#levels + 1] = y end
    for _, y in ipairs(levels) do
      prts.scene.light{ pos = { t.x, y + 2.4, t.z }, color = c, radius = t.inner * 2 }
    end
  end
  return top, stair_top, bands
end

-- A ring of curtain walls around a rectangular ward, with towers on it and gaps for gates.
-- R = { x0, x1, z0, z1 (the inner faces), thick, height, material, floor_material,
--       towers = { { x, z, radius, height, wall, stair, roof }, ... },
--       gates = { { side = "north"|"south"|"west"|"east", a0, a1 }, ... } }.
-- A tower blocks a wall where its circle crosses the wall's centre line; walls run between
-- towers and gates. Each tower gets a door from the ward on the ground and a door at the
-- wall-walk towards each wall that meets it. Returns the number of wall segments.
function C.curtain_ring(R)
  local t, h = R.thick, R.height
  local cx0, cx1, cz0, cz1 = R.x0 - t / 2, R.x1 + t / 2, R.z0 - t / 2, R.z1 + t / 2
  local mx, mz = (R.x0 + R.x1) / 2, (R.z0 + R.z1) / 2
  local sides = {
    { name = "north", axis = "x", c = cz0, lo = cx0, hi = cx1, inner = R.z0, outer = R.z0 - t },
    { name = "south", axis = "x", c = cz1, lo = cx0, hi = cx1, inner = R.z1, outer = R.z1 + t },
    { name = "west", axis = "z", c = cx0, lo = cz0, hi = cz1, inner = R.x0, outer = R.x0 - t },
    { name = "east", axis = "z", c = cx1, lo = cz0, hi = cz1, inner = R.x1, outer = R.x1 + t },
  }
  local doors = {}
  for i = 1, #R.towers do doors[i] = {} end
  local segments = 0
  for _, side in ipairs(sides) do
    local blocks = {}
    for i, tw in ipairs(R.towers) do
      local along = side.axis == "x" and tw.x or tw.z
      local across = side.axis == "x" and tw.z or tw.x
      local d = math.abs(across - side.c)
      local reach = tw.radius - (tw.wall or 1.5) / 2
      if d < reach - 0.5 and along >= side.lo - reach and along <= side.hi + reach then
        local e = math.sqrt(reach * reach - d * d)
        blocks[#blocks + 1] = { a0 = along - e, a1 = along + e, tower = i, along = along, across = across, e = e }
      end
    end
    for _, g in ipairs(R.gates or {}) do
      if g.side == side.name then blocks[#blocks + 1] = { a0 = g.a0, a1 = g.a1 } end
    end
    table.sort(blocks, function(p, q) return p.a0 < q.a0 end)
    for k = 1, #blocks - 1 do
      local a, b = blocks[k].a1, blocks[k + 1].a0
      if b - a > 0.05 then
        C.curtain(R.material, side.axis, a, b, side.inner, side.outer, h)
        segments = segments + 1
        -- the towers at both ends get a door at the wall-walk, facing this wall
        for _, pair in ipairs({ { blocks[k], 1 }, { blocks[k + 1], -1 } }) do
          local blk, dir = pair[1], pair[2]
          if blk.tower then
            local dx, dz = dir * blk.e, side.c - blk.across
            if side.axis == "z" then dx, dz = side.c - blk.across, dir * blk.e end
            local list = doors[blk.tower]
            list[#list + 1] = { side = C.side_towards(16, dx, dz), y = h }
          end
        end
      end
    end
  end
  for i, tw in ipairs(R.towers) do
    local list = doors[i]
    list[#list + 1] = { side = C.side_towards(16, mx - tw.x, mz - tw.z), y = 0 }
    C.round_tower{ x = tw.x, z = tw.z, radius = tw.radius, wall = tw.wall, height = tw.height, material = R.material,
                   floor_material = R.floor_material, floors = { h }, doors = list, stair = tw.stair, roof = tw.roof }
  end
  return segments
end

-- A straight stair against a wall, from the ground (base) up to `top` (a wall-walk).
-- s = { axis, sign, start, w0, w1, base, top }. Returns the number of steps.
function C.wall_stair(material, s)
  local steps = math.ceil((s.top - s.base) / 0.25)
  B.stairs(material, { axis = s.axis, sign = s.sign, start = s.start, w0 = s.w0, w1 = s.w1, steps = steps,
                       rise = (s.top - s.base) / steps, run = 0.3, base = s.base, solid = true })
  return steps
end

-- A gatehouse across a wall that runs along x: a block from x - half to x + half and from
-- z0 to z1 (through the wall), as high as the wall-walk, with a passage through it, two drum
-- towers on the outer side and a portcullis.
-- g = { name, x, z0, z1, half, height, outward (+1: outside is towards +z, -1: towards -z),
--       passage = { half = 2.5, height = 5.5 }, tower = { radius, height }, material,
--       floor_material, gate_material }. Returns the portcullis entity.
function C.gatehouse(g)
  local pw, ph = g.passage.half, g.passage.height
  local x0, x1 = g.x - g.half, g.x + g.half
  local m = g.material
  B.box(m, x0, -1, g.z0, g.x - pw, g.height, g.z1)
  B.box(m, g.x + pw, -1, g.z0, x1, g.height, g.z1)
  B.box(m, g.x - pw, ph, g.z0, g.x + pw, g.height, g.z1)
  B.box(g.floor_material or m, g.x - pw, -0.5, g.z0, g.x + pw, 0.02, g.z1, { solid = false })
  -- battlements on both faces of the gatehouse top
  local outer_z = g.outward > 0 and g.z1 or g.z0
  local inner_z = g.outward > 0 and g.z0 or g.z1
  C.battlements(m, "x", x0, x1, math.min(outer_z, outer_z - g.outward * C.PARAPET_THICK),
                math.max(outer_z, outer_z - g.outward * C.PARAPET_THICK), g.height)
  -- drum towers flanking the gate on the outside, entered from the gatehouse top
  local r = g.tower.radius
  local tz = outer_z
  for _, sx in ipairs({ -1, 1 }) do
    local tx = g.x + sx * (g.half - r * 0.6)
    C.round_tower{ x = tx, z = tz, radius = r, height = g.tower.height, material = m,
                   floor_material = g.floor_material, floors = { g.height },
                   doors = { { side = C.side_towards(16, -sx * 0.3, -g.outward), y = g.height } } }
  end
  -- the portcullis, a little inside the outer face, and its winch inside the passage
  local pz = outer_z - g.outward * 1.5
  return Portcullis.make{ name = g.name, x = g.x, z = pz, width = 2 * pw, height = ph, material = g.gate_material or "gate",
                          winch = { g.x + pw - 0.35, 1.2, pz - g.outward * 2.5 } }
end

-- A hall with stone or plaster walls, a door and a gable roof. h = { x0, z0, x1, z1, height,
-- ridge (roof height above the walls), door = { side = "x0"|"x1"|"z0"|"z1", at, width },
-- material, roof_material, floor_material }. The ridge runs along the longer side.
function C.hall(h)
  local t = 0.5
  local m = h.material
  local dw = (h.door and h.door.width or 1.6) / 2
  local function openings(side)
    if h.door and h.door.side == side then return { { from = h.door.at - dw, to = h.door.at + dw, bottom = 0, top = 2.6 } } end
    return nil
  end
  B.wall(m, "x", h.x0, h.x1, h.z0, h.z0 + t, -0.5, h.height, openings("z0"))
  B.wall(m, "x", h.x0, h.x1, h.z1 - t, h.z1, -0.5, h.height, openings("z1"))
  B.wall(m, "z", h.z0 + t, h.z1 - t, h.x0, h.x0 + t, -0.5, h.height, openings("x0"))
  B.wall(m, "z", h.z0 + t, h.z1 - t, h.x1 - t, h.x1, -0.5, h.height, openings("x1"))
  B.box(h.floor_material or "planks", h.x0 + t, -0.5, h.z0 + t, h.x1 - t, 0.05, h.z1 - t, { solid = false })
  local roof = h.roof_material or "slate"
  local y0, y1 = h.height, h.height + (h.ridge or 3)
  if h.x1 - h.x0 >= h.z1 - h.z0 then
    local mid = (h.z0 + h.z1) / 2
    B.wedge(roof, h.x0 - 0.3, y0, h.z0 - 0.4, h.x1 + 0.3, y1, mid, { axis = "z", sign = 1, solid = false })
    B.wedge(roof, h.x0 - 0.3, y0, mid, h.x1 + 0.3, y1, h.z1 + 0.4, { axis = "z", sign = -1, solid = false })
  else
    local mid = (h.x0 + h.x1) / 2
    B.wedge(roof, h.x0 - 0.4, y0, h.z0 - 0.3, mid, y1, h.z1 + 0.3, { axis = "x", sign = 1, solid = false })
    B.wedge(roof, mid, y0, h.z0 - 0.3, h.x1 + 0.4, y1, h.z1 + 0.3, { axis = "x", sign = -1, solid = false })
  end
  prts.scene.light{ pos = { (h.x0 + h.x1) / 2, h.height - 0.6, (h.z0 + h.z1) / 2 }, color = { 1.1, 0.75, 0.45 },
                    radius = math.max(h.x1 - h.x0, h.z1 - h.z0) * 0.8 }
end

return C
