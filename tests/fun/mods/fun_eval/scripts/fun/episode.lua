-- One fun-audit episode: a persona plays Harrowmark from the road before the King's Gate for a
-- fixed time and the telemetry records what happened. Planning per persona:
--   oracle  the order of goals with the cheapest total route (all orders tried), true costs
--   human   the nearest goal it can see next, routes by its own (noisy) idea of their cost
--   spam    straight at the nearest goal, mashing use
--   novice  random inputs
-- MIT License.
local Bot = require("fun.bot")
local Goals = require("fun.goals")
local N = require("fun.nav")
local Tel = require("fun.telemetry")

local E = {}
E.SECONDS = 300

local function goal_nodes(g)
  if g.id == "war_room" then return { "war_lift", "war_stair" } end
  return { g.node }
end

-- Cheapest cost from `from` to a node of goal g, with the node.
local function cost_to(from, g, costs, memo, blocked)
  local best, node = math.huge, nil
  for _, n in ipairs(goal_nodes(g)) do
    local key = from .. ">" .. n
    if not memo[key] then memo[key] = select(2, N.route(from, n, costs, blocked)) end
    if memo[key] < best then best, node = memo[key], n end
  end
  return best, node
end

-- Every order of the goals left; the one with the least total cost.
local function best_order(from, left, costs, blocked)
  local memo, best, best_seq = {}, math.huge, nil
  local seq, used = {}, {}
  local function go(at, total)
    if total >= best then return end
    if #seq == #left then best, best_seq = total, { table.unpack(seq) } return end
    for i, g in ipairs(left) do
      if not used[i] then
        local c, node = cost_to(at, g, costs, memo, blocked)
        used[i] = true
        seq[#seq + 1] = { goal = g, node = node }
        go(node, total + c)
        seq[#seq] = nil
        used[i] = false
      end
    end
  end
  go(from, 0)
  return best_seq or {}, best
end

local function nearest(s, left)
  local best, bd = nil, math.huge
  for _, g in ipairs(left) do
    local d = (g.at[1] - s.x) ^ 2 + (g.at[2] - s.y) ^ 2 + (g.at[3] - s.z) ^ 2
    if d < bd then best, bd = g, d end
  end
  return best
end

local function reach(bot, g, node, costs)
  if N.travel(bot, node, costs) and g.id == "castellan" then bot:use("npc.hall_guard", 0, 1.3, 0) end
end

local POLICIES = {}

function POLICIES.oracle(bot)
  local tries = {}
  while true do
    local left = bot.goals.left()
    local seq = best_order(bot.node, left, nil, bot.blocked)
    local step = seq[1]
    if not step then bot.stop = "unreachable" error(Bot.STOP, 0) end
    tries[step.goal.id] = (tries[step.goal.id] or 0) + 1
    if tries[step.goal.id] > 3 then bot.stop = "unreachable" error(Bot.STOP, 0) end
    reach(bot, step.goal, step.node)
  end
end

function POLICIES.human(bot)
  -- the human's own idea of how long each way takes (fixed for the episode)
  local costs = {}
  for i, e in ipairs(N.edges) do costs[i] = e[3] * math.exp(Bot.gauss(0.35)) end
  local tries = {}
  while true do
    local g = nearest(bot.s, bot.goals.left())
    tries[g.id] = (tries[g.id] or 0) + 1
    if tries[g.id] > 3 then bot.stop = "unreachable" error(Bot.STOP, 0) end
    local _, node = cost_to(bot.node, g, costs, {}, bot.blocked)
    bot.tel.emit("choice", { goal = g.id, node = node })
    reach(bot, g, node, costs)
  end
end

function POLICIES.spam(bot)
  local n = 0
  while true do
    local g = nearest(bot.s, bot.goals.left())
    bot:turn_to(Bot.yaw_towards(g.at[1] - bot.s.x, g.at[3] - bot.s.z))
    bot:hold("move_forward")
    bot:speed_keys(100)
    n = n + 1
    if n % 4 == 0 then bot:press("use", "spam") end
    bot:step()
  end
end

local MOVES = { "move_forward", "move_back", "move_left", "move_right", false }
local VERBS = { "jump", "use", "crouch" }
function POLICIES.novice(bot)
  while true do
    local ticks = math.random(8, 15) -- 2 to 4 decisions a second
    local move = MOVES[math.random(#MOVES)]
    local run = math.random() < 0.5
    local turn = (math.random() * 2 - 1) * 1.5
    if math.random() < 0.3 then bot:press(VERBS[math.random(#VERBS)], "random") end
    for _ = 1, ticks do
      if move then bot:hold(move) end
      if run then bot:hold("run") end
      bot.t.look(bot.s.yaw + turn / ticks, 0)
      bot:step()
    end
  end
end

-- Plays one episode. Returns the goals done and how it stopped.
function E.run(t, persona, seconds)
  local map = require("maps.proving_ground")
  Tel.install(t)
  local tracker = Goals.tracker(map, Tel)
  t.place("start")
  t.ticks(2)
  local bot = Bot.new(t, persona, Tel, tracker, seconds or E.SECONDS)
  bot.node = "start"
  Tel.emit("episode", { persona = persona, goals = #tracker.list, seconds = seconds or E.SECONDS })
  local ok, err = xpcall(POLICIES[persona], function(e)
    if e == Bot.STOP or e == Bot.BLOCKED then return e end
    return debug.traceback(tostring(e), 2)
  end, bot)
  if not ok and err == Bot.BLOCKED then bot.stop = "stuck"
  elseif not ok and err ~= Bot.STOP then error(err, 0) end
  Tel.emit("end", { persona = persona, done = tracker.count, goals = #tracker.list, stop = bot.stop or "?",
                    stuck = bot.stuck, ticks = prts.time.tick() })
  return tracker.count, bot.stop
end

-- The survey: the oracle walks every way of the route graph, there and back, and lists the
-- ways it could not get through (a level change that breaks a way shows up here).
function E.survey(t, seconds)
  local map = require("maps.proving_ground")
  Tel.install(t)
  local tracker = Goals.tracker(map, Tel)
  t.place("start")
  t.ticks(2)
  local bot = Bot.new(t, "oracle", Tel, tracker, seconds or 1500)
  bot.goals = { check = tracker.check, all = function() return false end, left = tracker.left }
  bot.node = "start"
  local blocked = {}
  local ok, err = pcall(function()
    for i, e in ipairs(N.edges) do
      if bot.node ~= e[1] and not N.travel(bot, e[1]) then
        blocked[#blocked + 1] = "the way to " .. e[1]
      else
        for _, back in ipairs({ false, true }) do
          local walked, why = pcall(e[4], bot, back)
          if not walked then
            if why ~= Bot.BLOCKED then error(why, 0) end
            blocked[#blocked + 1] = e[1] .. " - " .. e[2] .. (back and " (back)" or "")
            bot.blocked[i] = true
            bot.node = N.nearest(bot.s)
            break
          end
          bot.node = back and e[1] or e[2]
        end
      end
    end
  end)
  if not ok and err ~= Bot.STOP then error(err, 0) end
  Tel.emit("survey", { blocked = blocked, ways = #N.edges, stop = bot.stop or "done", ticks = prts.time.tick() })
  return blocked, bot.stop
end

E.best_order = best_order
return E
