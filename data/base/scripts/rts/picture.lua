-- The picture of the battlefield, per side: no side sees the whole map. Every unit and finished
-- structure looks around it (its sight radius); enemies in sight are "seen". A sighting that is
-- not renewed becomes "old" after a few seconds, and is forgotten after two minutes.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local P = {}
P.SEEN_FOR = 90      -- ticks a sighting stays "seen" without being renewed
P.FORGET = 3600      -- ticks before an old sighting is dropped

-- What a side's eyes are: { x, z, r } for each living unit and finished structure.
function P.eyes(side_id)
  local out = {}
  for _, e in ipairs(S.units(side_id)) do
    local x, _, z = S.pos(e)
    out[#out + 1] = { x, z, D.unit(S.data(e).def).sight }
  end
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    local x, _, z = S.pos(e)
    local def = D.structure(S.data(e).def)
    out[#out + 1] = { x, z, (def.sight or 20) + def.size[1] / 2 }
  end
  return out
end

-- Refreshes a side's sightings (every half second or so).
function P.update(side_id, now)
  local side = S.side(side_id)
  local pic = side.picture
  local eyes = P.eyes(side_id)
  for _, eye in ipairs(eyes) do
    for _, e in ipairs(prts.agent.near(eye[1], eye[2], eye[3], 64)) do
      local d = S.data(e)
      if d.kind == "unit" and d.side ~= side_id then
        local x, _, z = S.pos(e)
        pic[e] = { x = x, z = z, def = d.def, what = "unit", side = d.side, seen = now }
      end
    end
  end
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local d = S.data(e)
      if d.side ~= side_id then
        for _, eye in ipairs(eyes) do
          if S.rect_dist(eye[1], eye[2], d.fp) <= eye[3] then
            local x, _, z = S.pos(e)
            pic[e] = { x = x, z = z, def = d.def, what = "structure", side = d.side, seen = now, built = d.built }
            break
          end
        end
      end
    end
  end
  -- forget what is long gone, or seen dead
  for e, s in prts.sorted_pairs(pic) do
    if now - s.seen > P.FORGET or (now - s.seen <= P.SEEN_FOR and not S.alive(e)) then pic[e] = nil end
  end
  return P.count(side_id)
end

-- "seen" | "old" for a sighting, and its age in seconds.
function P.state(s, now)
  local age = (now - s.seen) / 30
  return (now - s.seen <= P.SEEN_FOR) and "seen" or "old", age
end

-- The side's sightings as a list in handle order: { e, x, z, def, what, state, age }.
function P.list(side_id, now)
  local out = {}
  for e, s in prts.sorted_pairs(S.side(side_id).picture) do
    local st, age = P.state(s, now)
    out[#out + 1] = { e = e, x = s.x, z = s.z, def = s.def, what = s.what, state = st, age = age, side = s.side }
  end
  return out
end

function P.count(side_id)
  local n = 0
  for _ in pairs(S.side(side_id).picture) do n = n + 1 end
  return n
end

return P
