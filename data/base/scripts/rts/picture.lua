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
  local Storm = require("rts.storm")
  for _, e in ipairs(S.units(side_id)) do
    if not S.data(e).under then -- those in the tunnels see nothing of the field
      local x, y, z = S.pos(e)
      local u = D.unit(S.data(e).def)
      out[#out + 1] = { x, z, Storm.sight(y, u.sight), y, u.senses }
    end
  end
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    local x, _, z = S.pos(e)
    local def = D.structure(S.data(e).def)
    local sight = (def.sight or 20) + def.size[1] / 2
    if def.balloon and require("props.balloon").up(e) then sight = require("props.balloon").SIGHT end
    out[#out + 1] = { x, z, sight }
  end
  for _, e in ipairs(S.match().ships or {}) do -- ships look out too
    if S.alive(e) and S.data(e).side == side_id then
      local x, _, z = S.pos(e)
      out[#out + 1] = { x, z, D.ship(S.data(e).def).sight or 60 }
    end
  end
  for _, e in ipairs(require("rts.citadel").all()) do -- a woken citadel looks down past the clouds
    local cd = S.data(e)
    if cd.state == "woken" and cd.side == side_id then
      local x, _, z = S.pos(e)
      out[#out + 1] = { x, z, require("rts.citadel").SIGHT }
    end
  end
  local Sh = require("props.skyship")
  for _, name in ipairs(Sh.ships) do -- the side's skyships
    local e = prts.entity.find(name)
    if e and S.data(e).side == side_id then
      local x, y, z = S.pos(e)
      out[#out + 1] = { x, z, Storm.sight(y, Sh.kind(e).sight or 80), y }
    end
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
        local hidden = D.unit(d.def).hidden -- the hushfoot: seen only close, or by hounds
        if not hidden or eye[5] or S.dist(eye[1], eye[2], x, z) <= hidden then
          pic[e] = { x = x, z = z, def = d.def, what = "unit", side = d.side, seen = now }
        end
      end
    end
  end
  for _, e in ipairs(S.match().units) do -- lit by glowstone shot, or shown by Crystal Sight
    if S.alive(e) then
      local d = S.data(e)
      if d.side ~= side_id and d.lit and (d.lit[side_id] or 0) > now then
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
  for _, e in ipairs(S.match().ships or {}) do -- enemy ships
    if S.alive(e) and S.data(e).side ~= side_id then
      local x, _, z = S.pos(e)
      for _, eye in ipairs(eyes) do
        if S.dist(eye[1], eye[2], x, z) <= eye[3] then
          pic[e] = { x = x, z = z, def = S.data(e).def, what = "ship", side = S.data(e).side, seen = now }
          break
        end
      end
    end
  end
  local Storm = require("rts.storm")
  for _, e in ipairs(require("rts.sky").flyers()) do -- enemy flyers (hidden in the storm)
    local d = S.data(e)
    if d.side ~= side_id then
      local x, y, z = S.pos(e)
      for _, eye in ipairs(eyes) do
        local dist = S.dist(eye[1], eye[2], x, z)
        if dist <= eye[3] and Storm.visible(eye[4], y, dist) then
          pic[e] = { x = x, z = z, def = d.def, what = "unit", side = d.side, seen = now }
          break
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
