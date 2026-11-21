-- Greybox building helpers: boxes, walls with openings, door and window frames, stair
-- flights, spiral stairs and ramps. MIT License.
-- Everything is axis-aligned and measured in meters.
local B = {}

function B.box(material, x0, y0, z0, x1, y1, z1, opts)
  local o = opts or {}
  return prts.scene.box{
    min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = material,
    solid = o.solid ~= false, visible = o.visible ~= false, entity = o.entity, tile = o.tile,
  }
end

-- A wall running along `axis` ("x" or "z") from a0 to a1, with thickness t0..t1 on the
-- other horizontal axis, from height y0 to y1. Openings are { from, to, bottom, top }
-- ranges along the wall. Openings with the same from/to are stacked vertically (a door on
-- one floor, a window above it); openings with different spans must not overlap.
-- The wall is split into boxes around the openings; returns how many boxes it made.
function B.wall(material, axis, a0, a1, t0, t1, y0, y1, openings)
  local by_span, spans = {}, {}
  for _, o in ipairs(openings or {}) do
    local key = o.from .. ":" .. o.to
    if not by_span[key] then
      by_span[key] = { from = o.from, to = o.to, holes = {} }
      spans[#spans + 1] = by_span[key]
    end
    table.insert(by_span[key].holes, o)
  end
  table.sort(spans, function(p, q) return p.from < q.from end)
  local count = 0
  local function piece(l0, l1, b0, b1)
    if l1 - l0 < 0.001 or b1 - b0 < 0.001 then return end
    count = count + 1
    if axis == "x" then
      B.box(material, l0, b0, t0, l1, b1, t1)
    else
      B.box(material, t0, b0, l0, t1, b1, l1)
    end
  end
  local cursor = a0
  for _, span in ipairs(spans) do
    piece(cursor, span.from, y0, y1)
    table.sort(span.holes, function(p, q) return p.bottom < q.bottom end)
    local b = y0
    for _, hole in ipairs(span.holes) do
      piece(span.from, span.to, b, hole.bottom)
      b = hole.top
    end
    piece(span.from, span.to, b, y1)
    cursor = span.to
  end
  piece(cursor, a1, y0, y1)
  return count
end

-- A straight flight of slab steps. `axis` is the walking direction ("x" or "z"), `sign`
-- +1 or -1, `start` the edge where the first step begins, w0..w1 the width range, `base`
-- the height of the floor below the first step.
function B.stairs(material, s)
  for i = 1, s.steps do
    local top = s.base + s.rise * i
    local e0 = s.start + s.sign * s.run * (i - 1)
    local e1 = e0 + s.sign * s.run
    local lo, hi = math.min(e0, e1), math.max(e0, e1)
    local bottom = s.solid and s.base or (top - (s.thickness or 0.35))
    if s.axis == "z" then
      B.box(material, s.w0, bottom, lo, s.w1, top, hi)
    else
      B.box(material, lo, bottom, s.w0, hi, top, s.w1)
    end
  end
  return s.base + s.rise * s.steps
end

-- A ramp filling the box x0..x1, y0..y1, z0..z1 that rises along opts.axis ("x" or "z") in
-- the direction opts.sign (1 or -1). Its collision is a run of invisible steps no higher than
-- 0.2 m (walkable with step-up); opts.solid = false leaves it out. Returns the step count.
function B.wedge(material, x0, y0, z0, x1, y1, z1, opts)
  local o = opts or {}
  local axis, sign = o.axis or "x", o.sign or 1
  prts.scene.wedge{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, axis = axis, sign = sign, material = material,
                    entity = o.entity, tile = o.tile }
  if o.solid == false then return 0 end
  local lo, hi = axis == "x" and x0 or z0, axis == "x" and x1 or z1
  local n = math.max(1, math.ceil((y1 - y0) / 0.2))
  for i = 1, n do
    local top = y0 + (y1 - y0) * (i - 0.5) / n
    local a, b = (i - 1) / n, i / n
    local s0 = sign > 0 and lo + (hi - lo) * a or hi - (hi - lo) * b
    local s1 = sign > 0 and lo + (hi - lo) * b or hi - (hi - lo) * a
    if axis == "x" then
      B.box(material, s0, y0, z0, s1, top, z1, { visible = false, entity = o.entity })
    else
      B.box(material, x0, y0, s0, x1, top, s1, { visible = false, entity = o.entity })
    end
  end
  return n
end

-- A spiral stair around a post. s = { center = { x, z }, base, steps, rise (0.25),
-- radius (0.9: distance of the step centres), size (0.8: step width), turn (pi/6 per step),
-- start (angle of the first step; 0 is +x, pi/2 is +z), thickness (0.3), post (material or
-- nil), post_radius (0.15), headroom (2.2: post height above the top step) }.
-- Returns the height of the top step and the angle after the last step.
function B.spiral(material, s)
  local cx, cz = s.center[1], s.center[2]
  local rise, radius, size = s.rise or 0.25, s.radius or 0.9, s.size or 0.8
  local turn, angle = s.turn or math.pi / 6, s.start or 0
  for i = 1, s.steps do
    local x, z = cx + radius * math.cos(angle), cz + radius * math.sin(angle)
    local top = s.base + rise * i
    B.box(material, x - size / 2, top - (s.thickness or 0.3), z - size / 2, x + size / 2, top, z + size / 2)
    angle = angle + turn
  end
  local top = s.base + rise * s.steps
  if s.post then
    local r = s.post_radius or 0.15
    B.box(s.post, cx - r, s.base, cz - r, cx + r, top + (s.headroom or 2.2), cz + r)
  end
  return top, angle
end

-- A wall with one door opening and a wooden frame around it. door = { from, to, height,
-- frame (material, default "oak"), trim (0.12) }. The frame stands 0.04 m proud of both wall
-- faces. Returns the opening as min, max corners (for a portal).
function B.door_frame(material, axis, a0, a1, t0, t1, y0, y1, door)
  B.wall(material, axis, a0, a1, t0, t1, y0, y1, { { from = door.from, to = door.to, bottom = y0, top = y0 + door.height } })
  local frame, w = door.frame or "oak", door.trim or 0.12
  local f0, f1 = t0 - 0.04, t1 + 0.04
  local function trim(l0, l1, b0, b1)
    if axis == "x" then B.box(frame, l0, b0, f0, l1, b1, f1) else B.box(frame, f0, b0, l0, f1, b1, l1) end
  end
  trim(door.from - w, door.from, y0, y0 + door.height + w)  -- jambs
  trim(door.to, door.to + w, y0, y0 + door.height + w)
  trim(door.from, door.to, y0 + door.height, y0 + door.height + w) -- lintel
  if axis == "x" then
    return { door.from, y0, t0 }, { door.to, y0 + door.height, t1 }
  end
  return { t0, y0, door.from }, { t1, y0 + door.height, door.to }
end

-- A wall with one window and a stone sill and lintel. window = { from, to, bottom, top, frame
-- (material, default "stone") }. The sill sticks out 0.12 m on both sides. Returns the opening
-- as min, max corners (for a portal).
function B.window_frame(material, axis, a0, a1, t0, t1, y0, y1, window)
  B.wall(material, axis, a0, a1, t0, t1, y0, y1, { window })
  local frame = window.frame or "stone"
  local f0, f1 = t0 - 0.12, t1 + 0.12
  local function bar(b0, b1)
    if axis == "x" then
      B.box(frame, window.from - 0.1, b0, f0, window.to + 0.1, b1, f1)
    else
      B.box(frame, f0, b0, window.from - 0.1, f1, b1, window.to + 0.1)
    end
  end
  bar(window.bottom - 0.1, window.bottom)  -- sill
  bar(window.top, window.top + 0.1)        -- lintel
  if axis == "x" then
    return { window.from, window.bottom, t0 }, { window.to, window.top, t1 }
  end
  return { t0, window.bottom, window.from }, { t1, window.top, window.to }
end

-- Small deterministic random generator for decoration, so layouts don't change with
-- the match seed.
function B.rng(seed)
  local state = seed % 2147483647
  if state <= 0 then state = state + 2147483646 end
  return function(lo, hi)
    state = (state * 48271) % 2147483647
    local f = state / 2147483647
    return lo + (hi - lo) * f
  end
end

return B
