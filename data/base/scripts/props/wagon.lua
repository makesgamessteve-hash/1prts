-- A supply wagon: a bed on four wheels and a driver's bench, a moving frame the commander rides on.
-- With no one on the bench it follows its road (a loop of points); from the bench (a post) the
-- move actions drive it: forward and back set the pace, left and right turn it. MIT License.
local Posts = require("lib.posts")

local W = {}
W.ROAD_SPEED = 2.5 -- m/s along the road
W.MAX_SPEED = 4.0  -- m/s when driven
W.TURN = 0.6       -- rad/s at full lock
W.ACCEL = 1.5      -- m/s per second

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- cfg = { name, road = { { x, z }, ... }, start = index (default 1) }. The wagon faces -z in its
-- own frame (forward), the bench is at the front.
function W.make(cfg)
  local p = cfg.road[cfg.start or 1]
  local q = cfg.road[(cfg.start or 1) % #cfg.road + 1]
  local yaw = math.atan(-(q[1] - p[1]), -(q[2] - p[2]))
  local e = prts.entity.spawn{ name = cfg.name, pos = { p[1], prts.scene.height(p[1], p[2]), p[2] }, yaw = yaw }
  -- the bed (stand on it), its sides, the bench, the wheels
  box(e, "planks", -1.0, 0.8, -1.6, 1.0, 1.0, 2.2)
  box(e, "oak", -1.05, 1.0, -1.6, -0.95, 1.5, 2.2)
  box(e, "oak", 0.95, 1.0, -1.6, 1.05, 1.5, 2.2)
  box(e, "oak", -1.0, 1.0, 2.1, 1.0, 1.5, 2.2)
  box(e, "oak", -0.8, 1.0, -1.6, 0.8, 1.45, -1.15) -- the driver's bench
  for _, w in ipairs({ { -1.15, -1.2 }, { 1.15, -1.2 }, { -1.15, 1.7 }, { 1.15, 1.7 } }) do
    box(e, "oak", w[1] - 0.06, 0.0, w[2] - 0.45, w[1] + 0.06, 0.9, w[2] + 0.45, false) -- a wheel
  end
  box(e, "oak", -1.2, 0.35, -1.25, 1.2, 0.5, -1.15, false) -- the axles
  box(e, "oak", -1.2, 0.35, 1.65, 1.2, 0.5, 1.75, false)
  box(e, "oak", -0.06, 0.6, -3.2, 0.06, 0.7, -1.6, false) -- the shaft
  local d = prts.entity.data(e)
  d.kind, d.road, d.next, d.speed, d.driven = "wagon", cfg.road, (cfg.start or 1) % #cfg.road + 1, 0, false
  d.post = prts.entity.name(Posts.make{ name = cfg.name .. ".bench", parent = e, pos = { 0, 1.75, -1.38 },
                                        kind = "wagon_seat", label = "driver's seat", seat = 0.3, size = { 1.4, 0.6, 0.5 },
                                        machine = cfg.name })
  return e
end

local function step(e, d, speed, turn, dt)
  local x, y, z = prts.entity.pos(e)
  local yaw = prts.entity.yaw(e) + turn * dt
  local fx, fz = -math.sin(yaw), -math.cos(yaw)
  local nx, nz = x + fx * speed * dt, z + fz * speed * dt
  local ny = prts.scene.height(nx, nz)
  prts.entity.place(e, { nx, ny, nz }, yaw)
  return nx, nz
end

-- One tick along the road: steer towards the next point, take the next one on arrival.
function W.follow_road(e, dt)
  local d = prts.entity.data(e)
  local x, _, z = prts.entity.pos(e)
  local p = d.road[d.next]
  local dx, dz = p[1] - x, p[2] - z
  if dx * dx + dz * dz < 4 then
    d.next = d.next % #d.road + 1
    return d.next
  end
  local want = math.atan(-dx, -dz)
  local diff = (want - prts.entity.yaw(e) + math.pi) % (2 * math.pi) - math.pi
  local turn = math.max(-W.TURN, math.min(W.TURN, diff * 2))
  d.speed = math.min(W.ROAD_SPEED, d.speed + W.ACCEL * dt)
  step(e, d, d.speed, turn, dt)
  return d.next
end

-- One tick driven from the bench: forward/back to the pace, left/right to the turn.
function W.drive(e, forward, turn, dt)
  local d = prts.entity.data(e)
  local want = math.max(-1, math.min(1, forward)) * W.MAX_SPEED
  if want > d.speed then d.speed = math.min(want, d.speed + W.ACCEL * dt)
  else d.speed = math.max(want, d.speed - W.ACCEL * 2 * dt) end
  -- turning needs the wagon to roll
  local t = -turn * W.TURN * math.min(1, math.abs(d.speed) / 1.5)
  step(e, d, d.speed, t, dt)
  return d.speed
end

W.wagons = {}

function W.track(e) W.wagons[#W.wagons + 1] = prts.entity.name(e) end

Posts.handlers.wagon_seat = {
  take = function(post) prts.entity.data(prts.entity.find(prts.entity.data(post).machine)).driven = true end,
  leave = function(post)
    local e = prts.entity.find(prts.entity.data(post).machine)
    if e then prts.entity.data(e).driven = false end
  end,
  tick = function(post, forward, turn)
    local e = prts.entity.find(prts.entity.data(post).machine)
    if e then W.drive(e, forward, turn, 1 / 30) end
  end,
}

prts.events.on("tick", "props.wagon", function()
  for _, name in ipairs(W.wagons) do
    local e = prts.entity.find(name)
    local d = e and prts.entity.data(e)
    if d and not d.driven and not d.parked then W.follow_road(e, 1 / 30) end
  end
end)
return W
