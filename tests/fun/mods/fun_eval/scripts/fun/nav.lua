-- Harrowmark's route graph for the playtest bots: places (nodes) joined by ways (edges), each
-- way with the moves that cover it in either direction (walking, a door, the keep's stairs or
-- hoist lift, the wall stair, the Raven Tower's spiral). Costs are seconds for a running
-- commander, measured with the oracle (tools/fun/README.md, "route_times"). MIT License.
local Bot = require("fun.bot")
local N = {}

N.nodes = {
  start = { 0, 0, 132 },
  outer = { 0, 0, 88 },
  inner = { 0, 0, 32 },
  wall_base = { -38.5, 0, 39.1 },
  wall_walk = { -22, 15, 41.5 },
  keep_front = { 0, 0, 12.6 },
  hall = { 0, 0.2, 6.8 },
  castellan = { 3.2, 0.2, -6.3 },
  lift_hall = { -6.6, 0.2, 0.0 },
  war_lift = { -5.6, 9.2, 0.0 },
  stair_foot = { 9.8, 0.2, 0.3 },
  war_stair = { 7.2, 9.2, -2.4 },
  keep_e = { 16, 0, 13 },
  keep_ne = { 16, 0, -16 },
  raven_front = { 0, 0, -38 },
  raven_top = { 0, 36, -49 },
}

-- A walk along points; a walk that gets blocked turns back along the points it reached.
local function walk(points)
  return function(bot, back)
    local list = {}
    for i, q in ipairs(points) do list[back and (#points + 1 - i) or i] = q end
    local reached = {}
    local ok, err = pcall(function()
      for _, q in ipairs(list) do
        bot:walk_to(q[1], q[2])
        reached[#reached + 1] = q
      end
    end)
    if not ok then
      if err ~= Bot.BLOCKED then error(err, 0) end
      for i = #reached - 1, 1, -1 do bot:walk_to(reached[i][1], reached[i][2]) end
      error(Bot.BLOCKED, 0)
    end
  end
end

local function door(name, outside, inside)
  return function(bot, back)
    local data = prts.entity.data(prts.entity.find(name))
    if not data.open then
      bot:walk_to(back and inside[1] or outside[1], back and inside[2] or outside[2])
      if not bot:use(name, 1.1, 1.6, 0) then error(Bot.BLOCKED, 0) end
      bot:wait(25)
    end
    bot:walk_to(back and outside[1] or inside[1], back and outside[2] or inside[2])
  end
end

-- The keep's switchback stairs: two flights a storey, from the great hall to the war room.
local FLIGHTS = { { 9.8, -7.4 }, { 7.2, -7.4 }, { 7.2, -2.4 }, { 9.8, -2.4 }, { 9.8, -7.4 }, { 7.2, -7.4 }, { 7.2, -2.4 } }
local function keep_stairs(bot, back)
  local list = {}
  if back then
    for i = #FLIGHTS - 1, 1, -1 do list[#list + 1] = FLIGHTS[i] end
    list[#list + 1] = { 9.8, 0.3 }
  else
    for _, q in ipairs(FLIGHTS) do list[#list + 1] = q end
  end
  local reached = 0
  local ok, err = pcall(function()
    for i, q in ipairs(list) do
      bot:walk_to(q[1], q[2], { radius = 0.5 })
      reached = i
    end
  end)
  if not ok then
    if err ~= Bot.BLOCKED then error(err, 0) end
    -- turn back the way it came, to where this way started
    for i = reached - 1, 1, -1 do bot:walk_to(list[i][1], list[i][2], { radius = 0.5 }) end
    if back then bot:walk_to(7.2, -2.4, { radius = 0.5 }) else bot:walk_to(9.8, 0.3) end
    error(Bot.BLOCKED, 0)
  end
end

-- The hoist lift: onto the platform, the lever, ride, out when the gate is up.
local function lift(bot, back)
  local t, c = bot.t, bot.t.commander()
  local top = 9.1
  local platform = prts.entity.find("keep.lift.platform")
  local here = back and #prts.entity.data(platform).floors or 1
  if prts.entity.data(platform).current ~= here or prts.entity.moving(platform) then
    bot:use("keep.lift.call" .. (here - 1)) -- call it to this floor
    bot:walk_dir(0, function()
      local gate = prts.entity.find("keep.lift.gate" .. (here - 1))
      return prts.entity.data(gate).open and not prts.entity.moving(gate) and not prts.entity.moving(platform)
    end, { ticks = 600, still = true })
  end
  bot:walk_dir(math.pi / 2, function() return t.parent(c) == "keep.lift.platform" end, { ticks = 150 })
  local inside = prts.time.tick() + 40
  bot:walk_dir(math.pi / 2, function() return prts.time.tick() >= inside end) -- on to the middle
  if not bot:use("keep.lift.lever") then error(Bot.BLOCKED, 0) end
  bot:wait(5)
  local function arrived()
    return not prts.entity.moving(platform) and (back and bot.s.y < 1 or bot.s.y > top)
  end
  bot:walk_dir(0, arrived, { ticks = 450, still = true }) -- ride until it stops
  bot:wait(60) -- the gate slides up
  -- out through the middle of the gate (the platform is wider than the opening)
  bot:walk_to(-7.0, 0.0, { radius = 0.3, walk = true })
  bot:walk_dir(-math.pi / 2, function() return t.parent(c) == "world" end, { ticks = 150 })
  bot:walk_to(back and N.nodes.lift_hall[1] + 1 or N.nodes.war_lift[1], 0)
end

-- The inner wall stair along the south wall, up to the wall-walk and down again.
local function wall_stair(bot, back)
  local h = 15
  if back then
    bot:walk_dir(0, function(s) return s.z < 39.4 end, { ticks = 90 })
    bot:walk_dir(math.pi / 2, function(s) return s.y < 0.05 end, { ticks = 900, run = true })
    bot:walk_to(-36, 35)
  else
    bot:walk_to(-38.0, 39.1, { radius = 0.4 })
    bot:walk_dir(-math.pi / 2, function(s) return s.y > h - 0.05 end, { ticks = 900, run = true })
    bot:walk_dir(math.pi, function(s) return s.z > 41.0 end, { ticks = 90 })
  end
end

-- The Raven Tower: through its door, round the spiral stair to the roof (or back down).
local function spiral(bot, back)
  local map = require("maps.proving_ground")
  local R = map.RAVEN
  local cx, cz, radius = R.x, R.z, 2.6
  local function follow(up, done)
    local limit = prts.time.tick() + 200 * 30
    while not done(bot.s) do
      local s = bot:perceived()
      local a = math.atan(s.z - cz, s.x - cx)
      local r = math.sqrt((s.x - cx) ^ 2 + (s.z - cz) ^ 2)
      local pull = (r - radius) * 2
      local tx, tz = -math.sin(a), math.cos(a)
      if not up then tx, tz = -tx, -tz end
      local dx, dz = tx - math.cos(a) * pull, tz - math.sin(a) * pull
      bot:turn_to(math.atan(-dx, -dz))
      bot:hold("move_forward")
      bot:hold("run")
      bot:step()
      if prts.time.tick() > limit then error(Bot.BLOCKED, 0) end
    end
  end
  if back then
    bot:walk_dir(0, function(s) return math.sqrt((s.x - cx) ^ 2 + (s.z - cz) ^ 2) < radius + 0.6 end, { ticks = 90 })
    follow(false, function(s) return s.y < 0.05 end)
    bot:walk_to(cx, cz + 4.6, { radius = 0.4 }) -- off the stair, in line with the door
    bot:walk_dir(math.pi, function(s) return s.z > cz + R.radius + 0.5 end, { ticks = 450 })
    bot:walk_to(N.nodes.raven_front[1], N.nodes.raven_front[3])
  else
    bot:walk_to(cx, cz + R.radius + 1.5, { radius = 0.5 })
    bot:walk_dir(0, function(s) return s.z < cz + 4.6 end, { ticks = 240 })
    follow(true, function(s) return s.y >= R.height - 0.05 end)
    bot:walk_dir(math.pi, function(s) return math.sqrt((s.x - cx) ^ 2 + (s.z - cz) ^ 2) > radius + 0.9 end, { ticks = 90 })
  end
end

-- { a, b, cost (s), move }
N.edges = {
  { "start", "outer", 7, walk({ { 0, 88 } }) },
  { "outer", "inner", 11, walk({ { 0, 32 } }) },
  { "inner", "wall_base", 9, walk({ { -30, 34.5 }, { -38.5, 39.1 } }) },
  { "wall_base", "wall_walk", 7, wall_stair },
  { "inner", "keep_front", 9, walk({ { 0, 12.6 } }) },
  { "keep_front", "hall", 3, door("keep.door", { 0, 11.6 }, { 0, 6.8 }) },
  { "hall", "castellan", 3, walk({ { 3.2, 7.2 }, { 3.2, -6.3 } }) },
  { "hall", "lift_hall", 3, walk({ { -0.5, 6.8 }, { -6.6, 6.9 }, { -6.6, 0.0 } }) },
  { "lift_hall", "war_lift", 12, lift },
  { "hall", "stair_foot", 2, walk({ { 3.2, 7.2 }, { 8.0, 0.5 }, { 9.8, 0.3 } }) },
  { "stair_foot", "war_stair", 14, keep_stairs },
  { "keep_front", "keep_e", 3, walk({ { 16, 13 } }) },
  { "inner", "keep_e", 7, walk({ { 16, 13 } }) },
  { "keep_e", "keep_ne", 5, walk({ { 16, -16 } }) },
  { "keep_ne", "raven_front", 6, walk({ { 0, -38 } }) },
  { "raven_front", "raven_top", 52, spiral },
}

-- Cheapest route between two nodes (Dijkstra): the list of { edge, backwards } and its cost.
function N.route(from, to, costs, blocked)
  local dist, prev, done = { [from] = 0 }, {}, {}
  while true do
    local best, bd = nil, math.huge
    for n, d in pairs(dist) do
      if not done[n] and (d < bd or (d == bd and best and n < best)) then best, bd = n, d end
    end
    if not best or best == to then break end
    done[best] = true
    for i, e in ipairs(N.edges) do
      local a, b = e[1], e[2]
      local other = (a == best and b) or (b == best and a) or nil
      if other and not done[other] and not (blocked and blocked[i]) then
        local nd = bd + ((costs and costs[i]) or e[3])
        if nd < (dist[other] or math.huge) then dist[other], prev[other] = nd, { i, best } end
      end
    end
  end
  if not dist[to] then return nil, math.huge end
  local steps, n = {}, to
  while n ~= from do
    local i, p = prev[n][1], prev[n][2]
    table.insert(steps, 1, { i, N.edges[i][1] ~= p })
    n = p
  end
  return steps, dist[to]
end

-- The node nearest to a sample (height counts four times: floors are far apart).
function N.nearest(s)
  local best, bd = nil, math.huge
  for name, p in pairs(N.nodes) do
    local d = (p[1] - s.x) ^ 2 + (4 * (p[2] - s.y)) ^ 2 + (p[3] - s.z) ^ 2
    if d < bd or (d == bd and name < best) then best, bd = name, d end
  end
  return best
end

-- Moves the bot along a route; the bot's `node` follows. A way that turns out to be blocked is
-- marked (the bot has turned back to where it started) and false is returned, to plan again.
function N.travel(bot, to, costs)
  local steps = N.route(bot.node, to, costs, bot.blocked)
  if not steps then bot.stop = "unreachable" error(Bot.STOP, 0) end
  for _, st in ipairs(steps) do
    local e = N.edges[st[1]]
    bot.tel.emit("edge", { a = e[1], b = e[2], back = st[2] })
    local t0 = prts.time.tick()
    local ok, err = pcall(e[4], bot, st[2])
    if not ok then
      if err ~= Bot.BLOCKED then error(err, 0) end
      bot.blocked[st[1]] = true
      bot.node = N.nearest(bot.s) -- wherever it got to
      bot.tel.emit("blocked", { a = e[1], b = e[2], back = st[2], ticks = prts.time.tick() - t0, at = bot.node })
      return false
    end
    bot.node = st[2] and e[1] or e[2]
    bot.tel.emit("edge_done", { a = e[1], b = e[2], back = st[2], ticks = prts.time.tick() - t0 })
  end
  return true
end

return N
