-- Greybox building helpers: boxes, walls with openings, stair flights. MIT License.
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
