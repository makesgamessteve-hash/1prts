-- A playtest bot: the commander driven through the action layer like a player would, with the
-- limits of its persona (tools/fun/README.md):
--   oracle  no delay, exact aim, reads the exact state, sprints whenever it can
--   human   sees its own position 200 ms late (+-35 ms jitter), turns at most 6 rad/s, aims
--           with a 2 degree error, presses 200 ms after deciding to, runs, sprints on long legs
--   spam    heads straight at the nearest goal, holding forward, run and sprint, mashing use
--   novice  random actions at 2-4 Hz, no idea where anything is
-- MIT License.
local B = {}
B.__index = B

B.PERSONAS = {
  oracle = { delay = 0, jitter = 0, aim = 0, turn = math.huge, radius = 0.45, press_delay = 0, sprint = "always" },
  human = { delay = 6, jitter = 1, aim = math.rad(2), turn = 6.0, radius = 0.8, press_delay = 6, sprint = "long" },
  spam = { delay = 0, jitter = 0, aim = 0, turn = math.huge, radius = 0.45, press_delay = 0, sprint = "always" },
  novice = { delay = 0, jitter = 0, aim = 0, turn = math.huge, radius = 0.45, press_delay = 0, sprint = "never" },
}

local HELD = { "run", "sprint", "crouch" } -- held verbs, logged when they start

B.STOP = {} -- error value that ends an episode (time is up, or every goal is done)
B.BLOCKED = {} -- error value of a move that cannot get through (the route planner tries another way)

-- Normally distributed noise from the seeded stream.
function B.gauss(sigma)
  if sigma == 0 then return 0 end
  local u1, u2 = math.max(1e-9, math.random()), math.random()
  return sigma * math.sqrt(-2 * math.log(u1)) * math.cos(2 * math.pi * u2)
end

function B.new(t, persona, telemetry, goals, seconds)
  local self = setmetatable({}, B)
  self.t, self.name, self.p = t, persona, B.PERSONAS[persona]
  self.tel, self.goals = telemetry, goals
  self.deadline = prts.time.tick() + math.floor(seconds * 30 + 0.5)
  self.s = telemetry.sample()
  self.stuck = 0
  self.presses = {}
  self.blocked = {}
  self.held, self.was_held = {}, {}
  return self
end

-- One simulation tick: sample, check the goals, stop when the time is up or all is done.
function B:step()
  -- held actions that start this tick count as one use of that verb
  for _, a in ipairs(HELD) do
    if self.held[a] and not self.was_held[a] then self.tel.emit("act", { action = a }) end
  end
  self.was_held, self.held = self.held, {}
  self.t.ticks(1)
  self.s = self.tel.sample()
  self.goals.check(self.s, self.t)
  if self.goals.all() then self.stop = "done" error(B.STOP, 0) end
  if prts.time.tick() >= self.deadline then self.stop = "time" error(B.STOP, 0) end
  return self.s
end

function B:wait(ticks)
  for _ = 1, ticks do self:step() end
end

-- What the bot believes its state is (late, for the human persona).
function B:perceived()
  local d = self.p.delay
  if d == 0 then return self.s end
  return self.tel.ago(d + math.random(-self.p.jitter, self.p.jitter))
end

function B:hold(action)
  self.t._hold(action, 2)
  self.held[action] = true
end

-- A discrete press: the action goes down for one tick (it must be up before, or the engine
-- sees no new press). Logged so the audit can tell accepted presses from dropped ones.
function B:press(action, why)
  self.t._hold(action, 1)
  self.tel.emit("press", { action = action, why = why or "" })
  self.presses[action] = (self.presses[action] or 0) + 1
end

-- Turns towards a direction on the ground, within the persona's turn rate.
function B:turn_to(yaw, pitch)
  local cur = self.s.yaw
  local d = (yaw - cur + math.pi) % (2 * math.pi) - math.pi
  local max = self.p.turn / 30
  if math.abs(d) > max then d = d > 0 and max or -max end
  self.t.look(cur + d, pitch or 0)
  return math.abs((yaw - cur + math.pi) % (2 * math.pi) - math.pi) - math.abs(d) < 1e-6
end

function B.yaw_towards(dx, dz) return math.atan(-dx, -dz) end

function B:speed_keys(leg)
  local sp = self.p.sprint
  self:hold("run")
  if sp == "always" or (sp == "long" and (leg or 0) > 20) then self:hold("sprint") end
end

-- Walks to x, z (any height: stairs and slopes carry the commander). `opts.stop(s)` ends the
-- walk early; `opts.radius` overrides the arrival radius; `opts.walk` walks instead of running.
-- Gets unstuck by backing off and side-stepping; gives up (ends the episode) after 4 tries.
function B:walk_to(x, z, opts)
  opts = opts or {}
  local radius = opts.radius or self.p.radius
  local s0 = self.s
  local leg = math.sqrt((x - s0.x) ^ 2 + (z - s0.z) ^ 2)
  local started, tries = prts.time.tick(), 0
  while true do
    local p = self:perceived()
    local dx, dz = x - p.x, z - p.z
    if dx * dx + dz * dz < radius * radius or (opts.stop and opts.stop(self.s)) then return true end
    self:turn_to(B.yaw_towards(dx, dz) + B.gauss(self.p.aim))
    self:hold("move_forward")
    -- a player slows down to walking pace to stop on a spot
    local careful = self.p.delay > 0 and dx * dx + dz * dz < 2.5 * 2.5
    if not opts.walk and not careful then self:speed_keys(leg) end
    self:step()
    local old = self.tel.ago(60)
    if prts.time.tick() - started > 60 and (self.s.x - old.x) ^ 2 + (self.s.z - old.z) ^ 2 < 0.09 then
      tries = tries + 1
      self.stuck = self.stuck + 1
      self.tel.emit("stuck", { x = self.s.x, y = self.s.y, z = self.s.z, tx = x, tz = z })
      if tries > 3 then error(B.BLOCKED, 0) end
      local side = math.random(2) == 1 and "move_left" or "move_right"
      for i = 1, 24 do
        self:hold(i <= 12 and "move_back" or side)
        self:step()
      end
      started = prts.time.tick()
    end
  end
end

-- Walks a list of { x, z } points.
function B:walk_path(points, opts)
  for _, q in ipairs(points) do self:walk_to(q[1], q[2], opts) end
end

-- Walks in the direction `yaw` until `cond(s)` holds (stairs, platforms, doorways).
function B:walk_dir(yaw, cond, opts)
  opts = opts or {}
  local limit = prts.time.tick() + (opts.ticks or 600)
  while not cond(self.s) do
    if not opts.still then
      self:turn_to(yaw + B.gauss(self.p.aim * 0.5))
      self:hold("move_forward")
      if opts.run then self:hold("run") end
    end
    self:step()
    if prts.time.tick() > limit then
      self.stuck = self.stuck + 1
      self.tel.emit("stuck", { x = self.s.x, y = self.s.y, z = self.s.z, dir = yaw })
      error(B.BLOCKED, 0)
    end
  end
end

-- Looks at an entity (a point offset in its frame) and uses it, as a player does: aims (with
-- the persona's error) and checks the prompt; with no prompt (out of reach) steps closer;
-- with the prompt, presses `use` after its reaction time. Returns true once it was used.
function B:use(name, ox, oy, oz, tries)
  local e = prts.entity.find(name)
  for _ = 1, tries or 8 do
    self.t.face(name, ox or 0, oy or 0, oz or 0)
    local yaw, pitch = prts.commander.view()
    self.t.look(yaw + B.gauss(self.p.aim), pitch + B.gauss(self.p.aim))
    self:step()
    if self.t.target() ~= name then
      self.tel.emit("no_prompt", { name = name })
      local ex, _, ez = prts.entity.pos(e)
      for _ = 1, 8 do
        self:turn_to(B.yaw_towards(ex - self.s.x, ez - self.s.z))
        self:hold("move_forward")
        self:step()
      end
    else
      self:wait(math.max(1, self.p.press_delay + math.random(-self.p.jitter, self.p.jitter)))
      local before = self.tel.counts.use
      self:press("use", name)
      self:step()
      if self.tel.counts.use > before then return true end
      self.tel.emit("use_miss", { name = name })
    end
  end
  return false
end

return B
