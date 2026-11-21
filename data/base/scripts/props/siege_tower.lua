-- The siege tower: a three-storey timber tower on wheels, written as a deck plan. Its crew pushes
-- it while the commander holds the push bar on the ground floor; it stops when its front meets
-- the wall. Stairs climb inside while it rolls; on the top floor a lever slides the bridge out
-- onto the wall-walk. The tower faces -z in its own frame. MIT License.
-- The crew push while told to (the push bar starts and stops them) until the tower meets the wall.
local Deck = require("lib.deckplan")
local Posts = require("lib.posts")
local Crew = require("lib.crew")

local T = {}
T.CELL = 1.2
T.LIFT = 0.6        -- the ground floor stands this high on its wheels
T.SPEED = 0.8       -- m/s with a full crew pushing
T.FRONT = 3.6       -- from the tower's centre to its front face
T.BRIDGE = 3.6      -- how far the bridge slides out
T.STOP = 0.4        -- the gap left to the wall

T.PLAN = {
  { y = 0, height = 3.2, rows = {
    "#####",
    "#p..#",
    "#p.S#",
    "#p.S#",
    "#p..#",
    "##D##",
  } },
  { y = 3.2, height = 3.2, rows = {
    "#####",
    "#mmm#",
    "#sm.#",
    "#sm.#",
    "#mmm#",
    "#####",
  } },
  { y = 6.4, height = 2.2, rows = {
    "#WDW#",
    "#ttt#",
    "#...#",
    "#...#",
    "#...#",
    "#####",
  } },
}

-- cfg = { name, pos = { x, z }, yaw, stop_at = function(tower) -> distance left to the wall }.
function T.make(cfg)
  local x, z = cfg.pos[1], cfg.pos[2]
  local e = prts.entity.spawn{ name = cfg.name, pos = { x, prts.scene.height(x, z), z }, yaw = cfg.yaw or 0 }
  local plan = Deck.compile{ entity = e, cell = T.CELL, origin = { -3.0, T.LIFT, -T.FRONT }, material = "planks",
                             wall = "oak", stair_material = "planks", levels = T.PLAN, prefix = cfg.name,
                             rooms = { p = "push", m = "middle", t = "top" } }
  -- wheels, a step up to the door, hides on the front
  for _, w in ipairs({ { -3.1, -2.6 }, { 3.1, -2.6 }, { -3.1, 2.6 }, { 3.1, 2.6 } }) do
    prts.scene.box{ min = { w[1] - 0.1, 0, w[2] - 0.6 }, max = { w[1] + 0.1, 1.2, w[2] + 0.6 }, material = "oak", entity = e,
                    solid = false }
  end
  prts.scene.box{ min = { -0.6, 0, 3.6 }, max = { 0.6, 0.3, 4.2 }, material = "planks", entity = e }
  -- lanterns on every floor
  for _, y in ipairs({ T.LIFT + 2.6, T.LIFT + 5.8, T.LIFT + 8.2 }) do
    prts.scene.light{ pos = { 0, y, 0 }, color = { 1.0, 0.78, 0.5 }, radius = 7, entity = e }
  end
  -- the bridge: a plank run lying on the top floor, slid out over the front when lowered
  local top = T.LIFT + 6.4
  local bridge = prts.entity.spawn{ name = cfg.name .. ".bridge", parent = e, pos = { 0, top, 0 } }
  prts.scene.box{ min = { -0.55, 0, -3.55 }, max = { 0.55, 0.12, 0.05 }, material = "planks", entity = bridge }
  local d = prts.entity.data(e)
  d.kind, d.plan, d.pushing, d.at_wall, d.bridge_down = "siege_tower", plan, false, false, false
  d.bridge = prts.entity.name(bridge)
  d.post = prts.entity.name(Posts.make{ name = cfg.name .. ".push_bar", parent = e, pos = { -1.2, T.LIFT + 1.1, -1.8 },
                                        kind = "push_bar", label = "push bar", size = { 0.4, 0.4, 3.0 }, machine = cfg.name })
  prts.scene.box{ min = { -1.25, T.LIFT + 1.0, -3.0 }, max = { -1.15, T.LIFT + 1.1, -0.6 }, material = "oak", entity = e,
                  solid = false }
  d.lever = prts.entity.name(Posts.make{ name = cfg.name .. ".bridge_lever", parent = e, pos = { 1.2, top + 1.0, -2.1 },
                                         kind = "bridge_lever", label = "bridge lever", machine = cfg.name })
  prts.scene.box{ min = { 1.15, top, -2.15 }, max = { 1.25, top + 1.1, -2.05 }, material = "oak", entity = e, solid = false }
  -- the crew: eight to push on the ground floor, two crossbowmen on the top
  local people = {}
  for i = 1, 8 do
    local col, row = (i - 1) % 2, (i - 1) // 2
    people[#people + 1] = { name = "pusher " .. i, room = "push", slot = { -2.1 + col * 1.0, T.LIFT, -2.2 + row * 1.0 } }
  end
  people[#people + 1] = { name = "Hal the bowman", room = "top", kind = "soldier", slot = { -1.2, top, -2.0 } }
  people[#people + 1] = { name = "Wyn the bowman", room = "top", kind = "soldier", slot = { 1.8, top, -2.0 } }
  Crew.new(e, { people = people, rooms = { push = 300, middle = 300, top = 300 } })
  Crew.track(e)
  T.towers[#T.towers + 1] = cfg.name
  T.stops[cfg.name] = cfg.stop_at
  return e
end

T.towers, T.stops = {}, {}

-- How fast the crew pushes now: with the pushers that are left, nothing in a wrecked room.
function T.push_speed(e)
  local d = prts.entity.data(e)
  if d.rooms.push.wrecked then return 0 end
  return T.SPEED * #Crew.living(e, "push") / 8
end

-- One tick of pushing: forward until the front meets the wall.
function T.push(e, dt)
  local d = prts.entity.data(e)
  if d.at_wall then
    d.pushing = false
    return 0
  end
  local speed = T.push_speed(e)
  local left = T.stops[prts.entity.name(e)] and T.stops[prts.entity.name(e)](e) or math.huge
  local step = math.min(speed * dt, math.max(0, left - T.STOP))
  local x, _, z = prts.entity.pos(e)
  local yaw = prts.entity.yaw(e)
  local nx, nz = x - math.sin(yaw) * step, z - math.cos(yaw) * step
  prts.entity.place(e, { nx, prts.scene.height(nx, nz), nz }, yaw)
  if left - step <= T.STOP + 1e-3 then
    d.at_wall, d.pushing = true, false
    prts.events.emit("siege_tower_at_wall", prts.entity.name(e))
  end
  return step
end

function T.lower_bridge(e)
  local d = prts.entity.data(e)
  if d.bridge_down or not d.at_wall or d.rooms.top.wrecked then return false end
  d.bridge_down = true
  local b = prts.entity.find(d.bridge)
  local _, y = prts.entity.local_pos(b)
  prts.entity.move_to(b, { 0, y, -T.BRIDGE }, 1.5)
  prts.events.emit("siege_bridge_lowered", prts.entity.name(e))
  return true
end

-- A wrecked room: scorched timbers on its floor, its post broken.
function T.wreck(e, room)
  local d = prts.entity.data(e)
  local r = d.plan.rooms[room]
  prts.scene.box{ min = { r.min[1] + 0.1, r.min[2], r.min[3] + 0.1 }, max = { r.max[1] - 0.1, r.min[2] + 0.03, r.max[3] - 0.1 },
                  material = "shadow", entity = e, solid = false }
  if room == "push" then prts.entity.data(prts.entity.find(d.post)).broken = true end
  if room == "top" then prts.entity.data(prts.entity.find(d.lever)).broken = true end
end

function T.damage(e, room, amount) return Crew.damage(e, room, amount, T.wreck) end

-- The push bar: the commander's word to the crew. Taking it sets them pushing, or stops them if
-- they are; either way the commander is free again at once (to climb while the tower rolls).
Posts.handlers.push_bar = {
  take = function(post)
    local e = prts.entity.find(prts.entity.data(post).machine)
    local d = prts.entity.data(e)
    d.pushing = not d.pushing and not d.at_wall
    prts.events.emit(d.pushing and "siege_push" or "siege_halt", prts.entity.name(e))
    Posts.leave()
  end,
}

Posts.handlers.bridge_lever = {
  take = function(post)
    local e = prts.entity.find(prts.entity.data(post).machine)
    if e then T.lower_bridge(e) end
    Posts.leave() -- a lever is pulled, not held
  end,
}

prts.events.on("tick", "props.siege_tower", function()
  for _, name in ipairs(T.towers) do
    local e = prts.entity.find(name)
    local d = e and prts.entity.data(e)
    if d and d.pushing then T.push(e, 1 / 30) end
  end
end)
return T
