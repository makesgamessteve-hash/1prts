-- Tunnels and deep caverns (M7): the tunnel kit and the tunnel roads. A map lays out nodes
-- (junction rooms, caverns, mouths up to the surface, the feet of deep shafts) and galleries
-- between them, which run north-south or east-west and may slope. Tu.build() then makes the
-- rock: each node and each gallery is an entity of boxes (rebuilt when a gallery is added or
-- collapses) and a cell, with portals where they meet and where a mouth's ramp comes down from
-- the field through a hole in the terrain. More than 3 m under the terrain the ground does not
-- hold anyone up, so these boxes are what is walked on; the navigation grid ignores them.
--
-- Companies travel the roads unseen (Tu.travel): they walk to a mouth, leave the field, follow
-- the galleries at their pace and come out at another mouth. Workers dig new galleries with a
-- mouth at the end (Tu.dig): the Gloam's delvers fast, others slowly; every metre is stone for
-- the Gloam, and digging rumbles on the surface above. Galleries collapse (Tu.collapse), are
-- opened again (Tu.open) and flood (Tu.flood). Underground the fog closes in, dark.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("lib.build")

local Tu = {}
Tu.ENTITY = "rts.tunnels"
Tu.W = 4             -- gallery width (one terrain quad), metres
Tu.H = 3.5           -- headroom
Tu.ROOM = 8          -- a junction room's side
Tu.RAMP = 0.5        -- a mouth's ramp rises this much per metre
Tu.LAMP_EVERY = 12
Tu.FOG = { near = 4, far = 70, color = { 0.09, 0.11, 0.12 } }
Tu.DIG = { rumble_every = 4, stone_per_metre = 1, depth = 10 }
Tu.FLOOD_DAMAGE = 2
Tu.WADE = 0.5

-- The tunnels' saved data: nodes, galleries, digs, collapsed and flooded state.
function Tu.data()
  local e = prts.entity.find(Tu.ENTITY)
  if not e then
    e = prts.entity.spawn{ name = Tu.ENTITY }
    local d = prts.entity.data(e)
    d.nodes, d.gals, d.digs, d.next_dig = {}, {}, {}, 1
    d.fog, d.rock = { 200, 1100, { 0.66, 0.70, 0.74 } }, "rock"
  end
  return prts.entity.data(e)
end

function Tu.exists() return prts.entity.find(Tu.ENTITY) ~= nil end

-- A coordinate on the terrain grid's quad centres (4 m), so mouths and shafts cut whole quads.
function Tu.snap(v) return math.floor(v / 4) * 4 + 2 end

local Fog = require("lib.fog")

-- The map's own fog, to come back to from underground.
function Tu.setup(cfg)
  local d = Tu.data()
  d.fog = cfg and cfg.fog or { 200, 1100, { 0.66, 0.70, 0.74 } }
  Fog.base(d.fog[1], d.fog[2], d.fog[3])
  d.rock = cfg and cfg.rock or "rock"
  return d
end

-- A node: id, x, z (snapped), y (absolute floor height; or depth = metres under the terrain),
-- kind "junction" | "cavern" | "mouth" | "shaft", and for mouths `dir` ("n", "s", "e", "w": where
-- the ramp climbs to), for caverns `size = { w, d, h }` and `grotto`.
function Tu.node(id, cfg)
  local d = Tu.data()
  local x, z = Tu.snap(cfg.x), Tu.snap(cfg.z)
  local y = cfg.y or math.floor(prts.scene.height(x, z) - (cfg.depth or 12))
  d.nodes[id] = { id = id, x = x, y = y, z = z, kind = cfg.kind or "junction", dir = cfg.dir, size = cfg.size,
                  grotto = cfg.grotto, links = {} }
  return d.nodes[id]
end

-- A gallery between two nodes in line (same x or same z).
function Tu.link(id, a, b)
  local d = Tu.data()
  local na, nb = d.nodes[a], d.nodes[b]
  assert(na and nb, "unknown node")
  assert(na.x == nb.x or na.z == nb.z, "galleries run north-south or east-west")
  d.gals[id] = { id = id, a = a, b = b, state = "open", len = math.abs(na.x - nb.x) + math.abs(na.z - nb.z) }
  na.links[#na.links + 1] = id
  nb.links[#nb.links + 1] = id
  return d.gals[id]
end

-- How far a node's room reaches from its centre (half its side), along x and z.
local function half(n)
  if n.kind == "cavern" then return n.size[1] / 2, n.size[2] / 2 end
  if n.kind == "mouth" or n.kind == "shaft" then return Tu.W / 2, Tu.W / 2 end
  return Tu.ROOM / 2, Tu.ROOM / 2
end
local function room_height(n) return n.kind == "cavern" and n.size[3] or Tu.H + 0.5 end

local function rock(e, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = Tu.data().rock, entity = e,
                  solid = solid ~= false, tile = 1 }
end

-- The side of node n a gallery to node o leaves from: "n" (-z), "s" (+z), "w" (-x) or "e" (+x).
local function side_towards(n, o)
  if o.x == n.x then return o.z < n.z and "n" or "s" end
  return o.x < n.x and "w" or "e"
end

local function other(g, id) return g.a == id and g.b or g.a end

-- Builds (or rebuilds) a node's room: floor, ceiling, and walls with a doorway for each gallery.
local function build_node(n)
  local d = Tu.data()
  local name = "tunnel.node." .. n.id
  local old = prts.entity.find(name)
  if old then prts.entity.despawn(old) end
  local e = prts.entity.spawn{ name = name, pos = { n.x, n.y, n.z } }
  local hx, hz = half(n)
  local h = room_height(n)
  local doors = { n = {}, s = {}, w = {}, e = {} }
  for _, gid in ipairs(n.links) do
    local o = d.nodes[other(d.gals[gid], n.id)]
    local side = side_towards(n, o)
    doors[side][#doors[side] + 1] = (side == "n" or side == "s") and 0 or 0 -- galleries meet rooms on their axis
  end
  if n.kind == "mouth" then doors[n.dir] = { 0 } end -- the ramp comes in here
  rock(e, -hx - 1, -1, -hz - 1, hx + 1, 0, hz + 1)                     -- floor
  if n.kind ~= "shaft" then rock(e, -hx - 1, h, -hz - 1, hx + 1, h + 1, hz + 1) end
  -- walls: each side split round its doorways (a gallery's width)
  local w2 = Tu.W / 2
  for side, gaps in pairs(doors) do
    local along = (side == "n" or side == "s") and hx or hz
    local cuts = {}
    for _, c in ipairs(gaps) do cuts[#cuts + 1] = { c - w2, c + w2 } end
    table.sort(cuts, function(p, q) return p[1] < q[1] end)
    local from = -along - 1
    local pieces = {}
    for _, c in ipairs(cuts) do
      pieces[#pieces + 1] = { from, c[1] }
      from = c[2]
    end
    pieces[#pieces + 1] = { from, along + 1 }
    for _, p in ipairs(pieces) do
      if p[2] - p[1] > 0.01 then
        if side == "n" then rock(e, p[1], 0, -hz - 1, p[2], h, -hz)
        elseif side == "s" then rock(e, p[1], 0, hz, p[2], h, hz + 1)
        elseif side == "w" then rock(e, -hx - 1, 0, p[1], -hx, h, p[2])
        else rock(e, hx, 0, p[1], hx + 1, h, p[2]) end
      end
    end
  end
  if n.kind == "cavern" then -- a glowstone grotto: crystals and their light
    prts.scene.light{ pos = { 0, h - 2, 0 }, color = { 0.45, 0.95, 0.9 }, radius = math.max(hx, hz) * 1.6, entity = e }
    for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
      prts.scene.light{ pos = { c[1] * hx * 0.6, 3, c[2] * hz * 0.6 }, color = { 0.4, 0.9, 0.85 }, radius = math.max(hx, hz),
                        entity = e }
    end
    if n.grotto then
      for i = 0, 5 do
        local a = i / 6 * 2 * math.pi
        prts.scene.prism{ base = { math.cos(a) * hx * 0.6, 0, math.sin(a) * hz * 0.6 }, radius = 0.8, top_radius = 0.1,
                          height = 3 + (i % 3), sides = 5, material = "aether_glow", entity = e }
      end
    end
  else
    prts.scene.light{ pos = { 0, Tu.H - 0.6, 0 }, color = { 0.5, 0.95, 0.85 }, radius = 10, entity = e }
  end
  return e
end

-- The ground point a mouth's ramp tops out at: { x, y, z } of its upper end, and its length.
local function ramp(n)
  local dx, dz = 0, 0
  if n.dir == "n" then dz = -1 elseif n.dir == "s" then dz = 1 elseif n.dir == "w" then dx = -1 else dx = 1 end
  local len = 4
  while len < 200 do
    local x, z = n.x + dx * (Tu.W / 2 + len), n.z + dz * (Tu.W / 2 + len)
    if n.y + Tu.RAMP * len >= prts.scene.height(x, z) then break end
    len = len + 4
  end
  local ex, ez = n.x + dx * (Tu.W / 2 + len), n.z + dz * (Tu.W / 2 + len)
  return { ex, prts.scene.height(ex, ez), ez }, len, dx, dz
end

-- A mouth's ramp: a hole cut in the terrain over it, the ramp's floor, its side walls and the
-- headwall over the gallery's end. Its top is where companies go in and come out.
local function build_mouth(n)
  local name = "tunnel.ramp." .. n.id
  local old = prts.entity.find(name)
  if old then prts.entity.despawn(old) end
  local top, len, dx, dz = ramp(n)
  local e = prts.entity.spawn{ name = name, pos = { 0, 0, 0 } }
  local w2 = Tu.W / 2
  local x0 = dx ~= 0 and math.min(n.x + dx * w2, n.x + dx * (w2 + len)) or n.x - w2
  local x1 = dx ~= 0 and math.max(n.x + dx * w2, n.x + dx * (w2 + len)) or n.x + w2
  local z0 = dz ~= 0 and math.min(n.z + dz * w2, n.z + dz * (w2 + len)) or n.z - w2
  local z1 = dz ~= 0 and math.max(n.z + dz * w2, n.z + dz * (w2 + len)) or n.z + w2
  prts.scene.hole{ min = { x0, z0 }, max = { x1, z1 } }
  local sign = (dx > 0 or dz > 0) and 1 or -1
  B.wedge(Tu.data().rock, x0, n.y, z0, x1, top[2], z1, { axis = dx ~= 0 and "x" or "z", sign = sign, entity = e })
  rock(e, x0, n.y - 1, z0, x1, n.y, z1)
  -- the hole's side walls (4 m pieces, each up to the ground beside it), and the headwall
  for k = 0, len - 4, 4 do
    local p0 = dx ~= 0 and n.x + dx * (w2 + k) or n.z + dz * (w2 + k)
    local p1 = p0 + 4 * (dx ~= 0 and dx or dz)
    local lo_p, hi_p = math.min(p0, p1), math.max(p0, p1)
    if dx ~= 0 then
      local g = math.max(prts.scene.height((p0 + p1) / 2, z0 - 1), prts.scene.height((p0 + p1) / 2, z1 + 1)) + 0.6
      rock(e, lo_p, n.y - 1, z0 - 1, hi_p, g, z0)
      rock(e, lo_p, n.y - 1, z1, hi_p, g, z1 + 1)
    else
      local g = math.max(prts.scene.height(x0 - 1, (p0 + p1) / 2), prts.scene.height(x1 + 1, (p0 + p1) / 2)) + 0.6
      rock(e, x0 - 1, n.y - 1, lo_p, x0, g, hi_p)
      rock(e, x1, n.y - 1, lo_p, x1 + 1, g, hi_p)
    end
  end
  if dx ~= 0 then
    local back = dx > 0 and x0 or x1
    rock(e, math.min(back, back - dx), n.y + Tu.H, z0, math.max(back, back - dx), prts.scene.height(back, n.z) + 0.6, z1)
  else
    local back = dz > 0 and z0 or z1
    rock(e, x0, n.y + Tu.H, math.min(back, back - dz), x1, prts.scene.height(n.x, back) + 0.6, math.max(back, back - dz))
  end
  -- a stone gate frame at the top
  local tx, tz = top[1], top[3]
  rock(e, tx - 2.6 * math.abs(dz) - 0.3 * math.abs(dx), top[2], tz - 2.6 * math.abs(dx) - 0.3 * math.abs(dz),
       tx - 2.2 * math.abs(dz) + 0.3 * math.abs(dx), top[2] + 3.4, tz - 2.2 * math.abs(dx) + 0.3 * math.abs(dz))
  rock(e, tx + 2.2 * math.abs(dz) - 0.3 * math.abs(dx), top[2], tz + 2.2 * math.abs(dx) - 0.3 * math.abs(dz),
       tx + 2.6 * math.abs(dz) + 0.3 * math.abs(dx), top[2] + 3.4, tz + 2.6 * math.abs(dx) + 0.3 * math.abs(dz))
  n.top = { tx + dx * 3, tz + dz * 3 }
  n.top_y = top[2]
  return e
end

-- A gallery's boxes: floor (a ramp if its ends differ in height), walls, ceiling and lamps.
local function build_gallery(g)
  local d = Tu.data()
  local name = "tunnel.gal." .. g.id
  local old = prts.entity.find(name)
  if old then prts.entity.despawn(old) end
  local na, nb = d.nodes[g.a], d.nodes[g.b]
  local e = prts.entity.spawn{ name = name, pos = { 0, 0, 0 } }
  local ahx, ahz = half(na)
  local bhx, bhz = half(nb)
  local w2 = Tu.W / 2
  local lo, hi = math.min(na.y, nb.y), math.max(na.y, nb.y)
  local x0, x1, z0, z1, axis, sign
  if na.z == nb.z then -- east-west
    local left, right = na.x < nb.x and na or nb, na.x < nb.x and nb or na
    x0 = left.x + (left == na and ahx or bhx)
    x1 = right.x - (right == na and ahx or bhx)
    z0, z1, axis = na.z - w2, na.z + w2, "x"
    sign = right.y >= left.y and 1 or -1
  else
    local near, far = na.z < nb.z and na or nb, na.z < nb.z and nb or na
    z0 = near.z + (near == na and ahz or bhz)
    z1 = far.z - (far == na and ahz or bhz)
    x0, x1, axis = na.x - w2, na.x + w2, "z"
    sign = far.y >= near.y and 1 or -1
  end
  if hi - lo > 0.05 then
    B.wedge(Tu.data().rock, x0, lo, z0, x1, hi, z1, { axis = axis, sign = sign, entity = e })
  end
  rock(e, x0, lo - 1, z0, x1, lo, z1)
  local top = hi + Tu.H
  if axis == "x" then
    rock(e, x0, lo - 1, z0 - 1, x1, top, z0)
    rock(e, x0, lo - 1, z1, x1, top, z1 + 1)
  else
    rock(e, x0 - 1, lo - 1, z0, x0, top, z1)
    rock(e, x1, lo - 1, z0, x1 + 1, top, z1)
  end
  rock(e, x0 - (axis == "z" and 1 or 0), top, z0 - (axis == "x" and 1 or 0), x1 + (axis == "z" and 1 or 0), top + 1,
       z1 + (axis == "x" and 1 or 0))
  local len = axis == "x" and x1 - x0 or z1 - z0
  for k = 1, math.floor(len / Tu.LAMP_EVERY) do
    local f = k * Tu.LAMP_EVERY / len
    local lx = axis == "x" and x0 + (x1 - x0) * f or (x0 + x1) / 2
    local lz = axis == "z" and z0 + (z1 - z0) * f or (z0 + z1) / 2
    prts.scene.box{ min = { lx - 0.2, top - 0.4, lz - 0.2 }, max = { lx + 0.2, top, lz + 0.2 }, material = "aether_glow",
                    entity = e, solid = false }
    prts.scene.light{ pos = { lx, top - 0.8, lz }, color = { 0.5, 0.95, 0.85 }, radius = 9, entity = e }
  end
  g.box = { x0, lo, z0, x1, top, z1 }
  if g.state == "collapsed" then -- rubble fills its middle
    local mx, mz = (x0 + x1) / 2, (z0 + z1) / 2
    rock(e, mx - (axis == "x" and 3 or w2), lo, mz - (axis == "z" and 3 or w2), mx + (axis == "x" and 3 or w2), top,
         mz + (axis == "z" and 3 or w2))
  elseif g.state == "flooded" then
    prts.scene.box{ min = { x0, lo, z0 }, max = { x1, lo + 1.1, z1 }, material = "water", entity = e, solid = false }
  end
  return e
end

-- Builds everything laid out so far (map building calls it once; later changes rebuild parts).
function Tu.build()
  local d = Tu.data()
  local cells = 0
  for _, n in prts.sorted_pairs(d.nodes) do
    if n.kind == "mouth" then build_mouth(n) end
  end
  for id, n in prts.sorted_pairs(d.nodes) do
    local hx, hz = half(n)
    if cells < 40 then
      prts.scene.cell{ name = "tunnel." .. id, min = { n.x - hx - 1, n.y - 1, n.z - hz - 1 },
                       max = { n.x + hx + 1, n.y + room_height(n) + 1, n.z + hz + 1 } }
      cells = cells + 1
      n.cell = "tunnel." .. id
    end
    prts.scene.cells{ n.cell or "outside", "outside" }
    build_node(n)
  end
  -- a portal at every doorway of a room with a cell, so sight and sound pass along the galleries
  for id, n in prts.sorted_pairs(d.nodes) do
    if n.cell then
      local hx, hz = half(n)
      local sides = {}
      for _, gid in ipairs(n.links) do sides[#sides + 1] = side_towards(n, d.nodes[other(d.gals[gid], id)]) end
      if n.kind == "mouth" then sides[#sides + 1] = n.dir end
      for k, side in ipairs(sides) do
        local ox = side == "e" and hx or side == "w" and -hx or 0
        local oz = side == "s" and hz or side == "n" and -hz or 0
        local wx, wz = (ox == 0) and Tu.W / 2 or 0.6, (oz == 0) and Tu.W / 2 or 0.6
        prts.scene.portal{ name = n.cell .. ".door" .. k, a = n.cell, b = "outside",
                           min = { n.x + ox - wx, n.y, n.z + oz - wz }, max = { n.x + ox + wx, n.y + Tu.H, n.z + oz + wz } }
      end
    end
  end
  for id, g in prts.sorted_pairs(d.gals) do
    local na, nb = d.nodes[g.a], d.nodes[g.b]
    prts.scene.cells{ na.cell or "outside", nb.cell or "outside", "outside" }
    build_gallery(g)
  end
  prts.scene.cells()
  return cells
end

-- The gallery a world point lies in, or nil.
function Tu.gallery_at(x, y, z)
  for id, g in prts.sorted_pairs(Tu.data().gals) do
    local b = g.box
    if b and x >= b[1] - 0.5 and x <= b[4] + 0.5 and z >= b[3] - 0.5 and z <= b[6] + 0.5 and y >= b[2] - 1 and y <= b[5] then
      return id, g
    end
  end
  return nil
end

-- Whether a world point lies under the ground (more than 3 m under the terrain, not in a hole).
function Tu.underground(x, y, z)
  if prts.scene.hole_at(x, z) then return false end
  return y < prts.scene.height(x, z) - 3
end

-- A route over open galleries from node a to node b: a list of node ids, or nil.
function Tu.route(a, b)
  local d = Tu.data()
  if not d.nodes[a] or not d.nodes[b] then return nil end
  local prev, queue, seen = {}, { a }, { [a] = true }
  local i = 1
  while queue[i] do
    local id = queue[i]
    i = i + 1
    if id == b then break end
    local links = {}
    for _, gid in ipairs(d.nodes[id].links) do links[#links + 1] = gid end
    table.sort(links)
    for _, gid in ipairs(links) do
      local g = d.gals[gid]
      if g.state ~= "collapsed" then
        local o = other(g, id)
        if not seen[o] then
          seen[o] = true
          prev[o] = { id, gid }
          queue[#queue + 1] = o
        end
      end
    end
  end
  if not seen[b] then return nil end
  local path, gals = { b }, {}
  local at = b
  while at ~= a do
    table.insert(path, 1, prev[at][1])
    table.insert(gals, 1, prev[at][2])
    at = prev[at][1]
  end
  return path, gals
end

-- The points a traveller follows from mouth a's top to mouth b's top along a route.
function Tu.points(path)
  local d = Tu.data()
  local pts = {}
  local first, last = d.nodes[path[1]], d.nodes[path[#path]]
  if first.top then pts[#pts + 1] = { first.top[1], first.top_y, first.top[2] } end
  for _, id in ipairs(path) do
    local n = d.nodes[id]
    pts[#pts + 1] = { n.x, n.y, n.z }
  end
  if last.top then pts[#pts + 1] = { last.top[1], last.top_y, last.top[2] } end
  return pts
end

-- Sends a company through the tunnels from mouth a to mouth b. Returns false if there is no way.
function Tu.travel(c, a, b)
  local d = Tu.data()
  local path, gals = Tu.route(a, b)
  if not path or not d.nodes[a].top or not d.nodes[b].top then return false end
  local top = d.nodes[a].top
  for _, u in ipairs(require("rts.companies").members(c)) do
    local ud = S.data(u)
    ud.tunnel = { path = path, gals = gals, stage = "to_mouth" }
    prts.agent.move(u, top[1], top[2])
  end
  c.order = { kind = "tunnel", x = d.nodes[b].top[1], z = d.nodes[b].top[2] }
  prts.events.emit("rts_tunnel_ordered", c.id, a, b)
  return true
end

-- Puts a unit under the ground on its route (it leaves the agents: unseen, untouchable).
local function go_under(u, ud)
  prts.agent.remove(u)
  ud.under = { pts = Tu.points(ud.tunnel.path), i = 2, gals = ud.tunnel.gals }
  ud.tunnel = nil
  prts.events.emit("rts_tunnel_in", u, ud.side)
end

-- Brings a unit up at the end of its route, back on the field.
local function come_up(u, ud)
  local p = ud.under.pts[#ud.under.pts]
  ud.under = nil
  prts.entity.place(u, { p[1], p[2], p[3] })
  local def = D.unit(ud.def)
  prts.agent.add(u, { speed = def.speed, radius = def.radius })
  prts.events.emit("rts_tunnel_out", u, ud.side)
end

-- Every tick: travellers walk to their mouths, follow the galleries and come up again.
function Tu.walk(dt)
  for _, u in ipairs(S.match().units) do
    if S.alive(u) then
      local ud = S.data(u)
      if ud.tunnel and ud.tunnel.stage == "to_mouth" then
        local top = Tu.data().nodes[ud.tunnel.path[1]].top
        local x, _, z = S.pos(u)
        if S.dist(x, z, top[1], top[2]) <= 2.5 then go_under(u, ud)
        elseif prts.time.tick() % 30 == 0 and not prts.agent.info(u).moving then prts.agent.move(u, top[1], top[2]) end
      elseif ud.under and ud.under.pts and ud.under.i <= #ud.under.pts then
        local p = ud.under.pts[ud.under.i]
        local x, y, z = prts.entity.pos(u)
        local dx, dy, dz = p[1] - x, p[2] - y, p[3] - z
        local dist = math.sqrt(dx * dx + dy * dy + dz * dz)
        local step = D.unit(ud.def).speed * (ud.slow or 1) * dt
        if dist <= step then
          prts.entity.place(u, { p[1], p[2], p[3] })
          ud.under.i = ud.under.i + 1
          if ud.under.i > #ud.under.pts and not ud.dig then come_up(u, ud) end
        else
          local yaw = math.atan(-dx, -dz)
          prts.entity.place(u, { x + dx / dist * step, y + dy / dist * step, z + dz / dist * step }, yaw)
        end
      end
    end
  end
end

-- The gallery a traveller is in now (between its last node and its next), or nil.
function Tu.travelling_in(ud)
  if not ud.under or not ud.under.gals then return nil end
  local leg = ud.under.i - 2 -- points: top, node1, node2, ...; leg k runs node k to node k+1
  return ud.under.gals[leg]
end

-- Collapses a gallery: rubble fills it, routes no longer use it, travellers in it are hurt and
-- turned back to where they came in.
function Tu.collapse(id)
  local d = Tu.data()
  local g = d.gals[id]
  if not g or g.state == "collapsed" then return false end
  g.state = "collapsed"
  build_gallery(g)
  local hurt = 0
  for _, u in ipairs(S.match().units) do
    if S.alive(u) then
      local ud = S.data(u)
      if Tu.travelling_in(ud) == id then
        ud.hp = math.floor(ud.hp / 2)
        local back = {}
        for k = ud.under.i - 1, 1, -1 do back[#back + 1] = ud.under.pts[k] end
        ud.under.pts, ud.under.i, ud.under.gals = back, 1, nil
        hurt = hurt + 1
      end
    end
  end
  local b = g.box
  Tu.rumble((b[1] + b[4]) / 2, (b[3] + b[6]) / 2)
  prts.events.emit("tunnel_collapsed", id, hurt)
  return true, hurt
end

-- Clears a collapsed gallery (Stonemeld).
function Tu.open(id)
  local g = Tu.data().gals[id]
  if not g or g.state ~= "collapsed" then return false end
  g.state = "open"
  build_gallery(g)
  prts.events.emit("tunnel_opened", id)
  return true
end

-- Floods a gallery: water stands in it.
function Tu.flood(id)
  local g = Tu.data().gals[id]
  if not g or g.state ~= "open" then return false end
  g.state = "flooded"
  build_gallery(g)
  prts.events.emit("tunnel_flooded", id)
  return true
end

-- Floods every gallery with an end within `reach` of deep water and under the water's level.
function Tu.flood_low(reach)
  local Wa = require("rts.water")
  local level = Wa.level()
  if not level then return 0 end
  local d, n = Tu.data(), 0
  for id, g in prts.sorted_pairs(d.gals) do
    for _, nid in ipairs({ g.a, g.b }) do
      local node = d.nodes[nid]
      local wet = false
      for _, off in ipairs({ { 0, 0 }, { reach, 0 }, { -reach, 0 }, { 0, reach }, { 0, -reach } }) do
        wet = wet or Wa.depth(node.x + off[1], node.z + off[2]) > 1
      end
      if wet and node.y < level and g.state == "open" then
        Tu.flood(id)
        n = n + 1
      end
    end
  end
  return n
end

-- The rumble of digging or collapse, heard on the surface above.
function Tu.rumble(x, z)
  if prts.audio then
    prts.audio.play{ sound = "sounds/rumble.wav", at = { x, prts.scene.height(x, z), z }, volume = 0.9, range = 160 }
  end
  prts.events.emit("tunnel_rumble", x, z)
end

-- Opens a dig from node `from` towards (x, z): a gallery along the nearer axis, ending in a new
-- mouth that climbs on in the same direction. Returns the dig's id.
function Tu.dig(side_id, from, x, z)
  local d = Tu.data()
  local n = d.nodes[from]
  if not n then return nil end
  local dx, dz = x - n.x, z - n.z
  local tx, tz, dir
  if math.abs(dx) >= math.abs(dz) then
    tx, tz, dir = Tu.snap(x), n.z, dx > 0 and "e" or "w"
  else
    tx, tz, dir = n.x, Tu.snap(z), dz > 0 and "s" or "n"
  end
  local id = d.next_dig
  d.next_dig = id + 1
  local len = math.abs(tx - n.x) + math.abs(tz - n.z)
  d.digs[id] = { id = id, side = side_id, from = from, x = tx, z = tz, dir = dir, len = len, done = 0, diggers = {} }
  prts.events.emit("tunnel_dig_opened", id, side_id)
  return id
end

-- Sets a worker digging: it walks to the nearest mouth that leads to the dig's start and goes under.
function Tu.assign(worker, dig_id)
  local d = Tu.data()
  local dg = d.digs[dig_id]
  local ud = S.data(worker)
  local x, _, z = S.pos(worker)
  local best, bd
  for id, n in prts.sorted_pairs(d.nodes) do
    if n.top and Tu.route(id, dg.from) then
      local dist = S.dist(x, z, n.top[1], n.top[2])
      if not bd or dist < bd then best, bd = id, dist end
    end
  end
  if not best then return false end
  ud.task = nil
  ud.dig = dig_id
  ud.tunnel = { path = Tu.route(best, dg.from), stage = "to_mouth" }
  local top = d.nodes[best].top
  prts.agent.move(worker, top[1], top[2])
  return true
end

-- Once a second: digs advance with their diggers; a finished dig becomes a gallery and a mouth.
function Tu.second(now)
  local d = Tu.data()
  for id, dg in prts.sorted_pairs(d.digs) do
    local rate, workers = 0, {}
    for _, u in ipairs(S.match().units) do
      if S.alive(u) then
        local ud = S.data(u)
        if ud.dig == id and ud.under and ud.under.i > #ud.under.pts then -- at the dig face
          rate = rate + (D.unit(ud.def).digs or 0.2)
          workers[#workers + 1] = u
        end
      end
    end
    if rate > 0 then
      local before = math.floor(dg.done)
      dg.done = math.min(dg.len, dg.done + rate)
      local side = S.side(dg.side)
      if side and side.faction == "gloam" then side.stone = (side.stone or 0) + (math.floor(dg.done) - before) * Tu.DIG.stone_per_metre end
      local n = d.nodes[dg.from]
      local f = dg.done / dg.len
      if now % (Tu.DIG.rumble_every * 30) == 0 then Tu.rumble(n.x + (dg.x - n.x) * f, n.z + (dg.z - n.z) * f) end
      if dg.done >= dg.len then Tu.finish_dig(id, workers) end
    end
  end
end

-- A finished dig: its gallery and mouth are built, and the diggers come up at the new mouth.
function Tu.finish_dig(id, workers)
  local d = Tu.data()
  local dg = d.digs[id]
  local from = d.nodes[dg.from]
  local nid = "dug." .. id
  Tu.node(nid, { x = dg.x, z = dg.z, y = from.y, kind = "mouth", dir = dg.dir })
  Tu.link("dug." .. id, dg.from, nid)
  build_mouth(d.nodes[nid])
  build_node(d.nodes[nid])
  build_node(from)
  build_gallery(d.gals["dug." .. id])
  local top = d.nodes[nid].top
  for i, u in ipairs(workers) do
    local ud = S.data(u)
    ud.dig, ud.under = nil, nil
    prts.entity.place(u, { top[1] + i, d.nodes[nid].top_y, top[2] })
    local def = D.unit(ud.def)
    prts.agent.add(u, { speed = def.speed, radius = def.radius })
  end
  d.digs[id] = nil
  prts.events.emit("rts_dug", id, dg.side, nid)
  return nid
end

-- Builds a node, its galleries and (for a mouth) its ramp at run time, after the map was built.
function Tu.build_part(node_ids, gal_ids)
  local d = Tu.data()
  for _, id in ipairs(node_ids) do
    if d.nodes[id].kind == "mouth" then build_mouth(d.nodes[id]) end
  end
  for _, id in ipairs(node_ids) do build_node(d.nodes[id]) end
  for _, id in ipairs(gal_ids or {}) do build_gallery(d.gals[id]) end
end

-- The Gloam's hall under a headquarters (Delve, Hollow Court, Sunken Throne): a cavern `depth`
-- metres down under the gatehouse, with a ramp (a mouth) up to the field north of it. Built once
-- per place; a later stage keeps the hall. Returns the hall's node.
function Tu.hall(structure, depth)
  local d = Tu.data()
  local x, _, z = prts.entity.pos(structure)
  local id = "hall." .. math.floor(x) .. "." .. math.floor(z)
  if d.nodes[id] then return d.nodes[id] end
  local y = math.floor(prts.scene.height(x, z) - depth)
  local hall = Tu.node(id, { x = x, z = z, y = y, kind = "cavern", size = { 16, 16, 5 } })
  local up = Tu.node(id .. ".up", { x = hall.x, z = hall.z - 14, y = y, kind = "mouth", dir = "n" })
  Tu.link(id .. ".stair", id, id .. ".up")
  Tu.build_part({ id .. ".up", id }, { id .. ".stair" })
  return hall
end

-- Where a headquarters' war table stands in its hall (in the structure's frame), or nil.
function Tu.hall_base(structure)
  local d = Tu.data()
  local x, y, z = prts.entity.pos(structure)
  local hall = d.nodes["hall." .. math.floor(x) .. "." .. math.floor(z)]
  if not hall then return nil end
  return { hall.x - x, hall.y - y, hall.z - z + 3 }
end

-- A deep mine shaft (tier 4): a hole down `depth` metres, a lift cage and a room at its foot,
-- joined to a tunnel node the map laid there (or standing alone). Returns the foot's node.
function Tu.shaft(structure, depth)
  local d = Tu.data()
  local x, _, z = prts.entity.pos(structure)
  local sx, sz = Tu.snap(x), Tu.snap(z)
  local foot
  for _, n in prts.sorted_pairs(d.nodes) do
    if n.kind == "shaft" and n.x == sx and n.z == sz then foot = n end
  end
  local g = prts.scene.height(sx, sz)
  if not foot then foot = Tu.node("shaft." .. sx .. "." .. sz, { x = sx, z = sz, y = math.floor(g - depth), kind = "shaft" }) end
  if foot.lift then return foot end
  foot.lift = "lift." .. foot.id
  prts.scene.hole{ min = { sx - 2, sz - 2 }, max = { sx + 2, sz + 2 } }
  local e = prts.entity.spawn{ name = "tunnel.shaftwall." .. foot.id, pos = { 0, 0, 0 } }
  local top = g + 0.8
  rock(e, sx - 3, foot.y + Tu.H, sz - 3, sx - 2, top, sz + 3)  -- the shaft's walls, up to the field
  rock(e, sx + 2, foot.y + Tu.H, sz - 3, sx + 3, top, sz + 3)
  rock(e, sx - 2, foot.y + Tu.H, sz - 3, sx + 2, top, sz - 2)
  rock(e, sx - 2, foot.y + Tu.H, sz + 2, sx + 2, top, sz + 3)
  build_node(foot)
  require("props.lift").make{ name = foot.lift, pos = { sx, g + 0.25, sz }, floors = { g + 0.25, foot.y + 0.25 },
                              size = { 3.6, 3.6 }, gate = sx - 1.95, material = "planks", gate_material = "iron",
                              lever_material = "oak" }
  return foot
end

-- Every tick: the commander underground sees the dark close in; in a flooded gallery they wade.
function Tu.tick(now)
  local d = Tu.data()
  local c = prts.commander.entity()
  if c then
    local x, y, z = prts.entity.pos(c)
    local _, _, eye = prts.commander.view()
    local under = Tu.underground(x, y + eye, z)
    if under ~= (d.dark == true) then
      d.dark = under
      if under then Fog.set("underground", Tu.FOG.near, Tu.FOG.far, Tu.FOG.color) else Fog.set("underground", nil) end
      prts.events.emit("underground", under)
    end
    local _, g = Tu.gallery_at(x, y, z)
    local wet = g ~= nil and g.state == "flooded"
    local st = prts.entity.find("commander.flooded")
    if wet and not st then
      st = prts.entity.spawn{ name = "commander.flooded" }
      local now_t = prts.commander.tune()
      prts.entity.data(st).saved = { walk_speed = now_t.walk_speed, run_speed = now_t.run_speed,
                                     sprint_speed = now_t.sprint_speed }
      prts.commander.tune{ walk_speed = now_t.walk_speed * Tu.WADE, run_speed = now_t.run_speed * Tu.WADE,
                           sprint_speed = now_t.sprint_speed * Tu.WADE }
      prts.events.emit("wading", true)
    elseif not wet and st then
      prts.commander.tune(prts.entity.data(st).saved)
      prts.entity.despawn(st)
      prts.events.emit("wading", false)
    end
  end
  if not prts.entity.find(S.REGISTRY) then return end
  Tu.walk(1 / 30)
  if now % 30 == 0 then
    Tu.second(now)
    for _, u in ipairs(S.match().units) do -- the flooded galleries drown the slow
      if S.alive(u) then
        local ud = S.data(u)
        local gid = Tu.travelling_in(ud)
        if gid and d.gals[gid].state == "flooded" then require("rts.combat").hit(u, Tu.FLOOD_DAMAGE) end
      end
    end
  end
end

prts.events.on("tick", "rts.tunnels", function(now)
  if Tu.exists() then Tu.tick(now) end
end)
return Tu
