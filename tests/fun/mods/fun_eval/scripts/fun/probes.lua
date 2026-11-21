-- The fun audit's measurements of feel and fairness, taken on fixed set-ups rather than in
-- free play (tools/fun/README.md):
--   latency    ticks from a press to the first change of state it causes
--   motion     the commander's speed tick by tick when starting and stopping, and the jump
--   timing     jumps pressed by human timing (+-100 ms) around landing and around a ledge
--   feedback   which feedback channels answer each kind of event
--   reach      how far around a usable thing the use ray still picks it
-- MIT License.
local Tel = require("fun.telemetry")

local P = {}

local FIELD = { 40, 0, 118 } -- open, flat ground east of the road, outside the walls

local function settle(t, x, y, z, yaw)
  t.place(x, y, z, yaw or 0, 0)
  t.ticks(3)
  Tel.sample()
end

local function step(t)
  t.ticks(1)
  return Tel.sample()
end

-- Ticks from the press (sent before tick 1) until cond(sample) holds; nil after `max`.
local function response(t, cond, max)
  for i = 1, max or 30 do
    if cond(step(t)) then return i end
  end
  return nil
end

function P.latency(t)
  local c = t.commander()
  local out = {}
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  local x0, _, z0 = t.pos(c)
  t._hold("move_forward", 10)
  out.move = response(t, function(s) return math.abs(s.x - x0) + math.abs(s.z - z0) > 1e-3 end)
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  t._hold("jump", 1)
  out.jump = response(t, function(s) return s.y > FIELD[2] + 1e-3 end)
  t.ticks(40)
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  local _, _, eye0 = prts.commander.view()
  t._hold("crouch", 10)
  out.crouch = response(t, function(s) return math.abs(s.eye - eye0) > 1e-3 end)
  t.ticks(20)
  -- the keep door
  local door = prts.entity.find("keep.door")
  t.place("keep_door")
  t.face("keep.door", 1.1, 1.6, 0)
  t.ticks(2)
  local yaw0 = prts.entity.yaw(door)
  t._hold("use", 1)
  out.use = response(t, function() return prts.entity.moving(door) or prts.entity.yaw(door) ~= yaw0 end)
  t.ticks(40)
  require("props.door").toggle(door) -- closed again for the next probes
  t.ticks(40)
  Tel.emit("probe_latency", out)
  return out
end

local function speed(s) return math.sqrt(s.vx * s.vx + s.vz * s.vz) end

function P.motion(t)
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  local start, stop = {}, {}
  for i = 1, 15 do
    t._hold("move_forward", 1)
    t._hold("run", 1)
    start[i] = speed(step(t))
  end
  for i = 1, 15 do stop[i] = speed(step(t)) end
  -- the jump: vertical speed tick by tick
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  t._hold("jump", 1)
  local vy = {}
  for i = 1, 30 do vy[i] = step(t).vy end
  Tel.emit("probe_motion", { start = start, stop = stop, jump_vy = vy, run_speed = 3.5 })
  return start, stop, vy
end

-- A ledge to run off: a 1 m block on the field.
local LEDGE = { 60, 1.0, 110, 118 } -- x, top, z of the edge, z of the far end
local function ledge(t)
  if not P.ledge_built then
    P.ledge_built = true
    prts.scene.box{ min = { LEDGE[1] - 2, 0, LEDGE[3] }, max = { LEDGE[1] + 2, LEDGE[2], LEDGE[4] }, material = "stone" }
  end
  settle(t, LEDGE[1], LEDGE[2], LEDGE[4] - 0.5, 0)
end

-- Ticks after a run starts until the commander leaves the ledge, and until a jump lands.
local function calibrate(t)
  ledge(t)
  local leave
  for i = 1, 120 do
    t._hold("move_forward", 2)
    t._hold("run", 2)
    if not step(t).ground then leave = i break end
  end
  t.ticks(30)
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  t._hold("jump", 1)
  local land, up = nil, false
  for i = 1, 60 do
    local s = step(t)
    if s.y > FIELD[2] + 0.01 then up = true end
    if up and s.ground then land = i break end
  end
  t.ticks(10)
  return leave, land
end

-- Did a jump start between tick `from` and `to` (counted from the trial's start)?
local function jumped(samples, from, to)
  for i = math.max(2, from), math.min(#samples, to) do
    if samples[i].vy > 1.0 and samples[i - 1].vy <= 1.0 then return true end
  end
  return false
end

-- Human timing: the press lands within +-100 ms of the moment it is aimed at.
function P.timing(t, trials)
  local leave, land = calibrate(t)
  local results = {}
  for n = 1, trials or 20 do
    -- coyote: run off the ledge, press jump around the moment the feet leave it
    local off = math.floor((math.random() * 200 - 100) / (1000 / 30) + 0.5)
    ledge(t)
    local samples = {}
    for i = 1, leave + 30 do
      t._hold("move_forward", 2)
      t._hold("run", 2)
      if i == leave + off then t._hold("jump", 1) end
      samples[i] = step(t)
    end
    local press = leave + off
    results[#results + 1] = { kind = "ledge", offset = off, accepted = jumped(samples, press, math.max(press, leave) + 4) }
    t.ticks(20)
    -- buffer: jump, then press again around the landing to jump straight on
    off = math.floor((math.random() * 200 - 100) / (1000 / 30) + 0.5)
    settle(t, FIELD[1], FIELD[2], FIELD[3])
    t._hold("jump", 1)
    samples = { step(t) }
    for i = 2, land + 30 do
      if i == land + off then t._hold("jump", 1) end
      samples[i] = step(t)
    end
    press = land + off
    results[#results + 1] = { kind = "landing", offset = off, accepted = jumped(samples, press, math.max(press, land) + 4) }
    t.ticks(40)
  end
  local dropped = 0
  for _, r in ipairs(results) do
    if not r.accepted then dropped = dropped + 1 end
    Tel.emit("probe_timing", r)
  end
  Tel.emit("probe_timing_summary", { trials = #results, dropped = dropped, leave = leave, land = land })
  return dropped / #results
end

-- Counts the feedback channels that answer within `window` ticks: sounds, new screen text,
-- things moving (`watch`), the camera (prts.commander.feel, once the engine has it) and
-- particles (Tel.counts.particles, once there are any).
local function counts() return { Tel.counts.audio, Tel.counts.ui, Tel.counts.particles or 0 } end

local function channels(t, window, watch, first, base)
  base = base or counts()
  local a0, u0, p0 = base[1], base[2], base[3]
  local moved, cam = false, false
  local start = {}
  for i, name in ipairs(watch or {}) do
    local e = prts.entity.find(name)
    local x, y, z = prts.entity.pos(e)
    start[i] = { e, x, y, z, prts.entity.yaw(e) }
  end
  for i = 1, window do
    local s = (i == 1 and first) and first or step(t)
    if (s.cam and math.abs(s.cam) > 1e-3) or (s.camp and math.abs(s.camp) > 1e-4) then cam = true end
    for _, st in ipairs(start) do
      local e = st[1]
      local x, y, z = prts.entity.pos(e)
      if prts.entity.moving(e) or math.abs(x - st[2]) + math.abs(y - st[3]) + math.abs(z - st[4]) > 1e-3
         or math.abs(prts.entity.yaw(e) - st[5]) > 1e-3 then moved = true end
    end
  end
  local got = {
    audio = Tel.counts.audio > a0, ui = Tel.counts.ui > u0, anim = moved, camera = cam,
    particles = (Tel.counts.particles or 0) > p0,
  }
  local n = 0
  for _, v in pairs(got) do if v then n = n + 1 end end
  got.n = n
  return got
end

local function report(event, impact, got)
  got.event, got.impact = event, impact
  Tel.emit("probe_feedback", got)
end

function P.feedback(t)
  local c = t.commander()
  -- running (footsteps, head motion)
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  for _ = 1, 10 do t._hold("move_forward", 2) t._hold("run", 2) step(t) end
  local function run_on() t._hold("move_forward", 2) t._hold("run", 2) end
  local a0, u0 = Tel.counts.audio, Tel.counts.ui
  local cam = false
  for _ = 1, 60 do
    run_on()
    local s = step(t)
    if s.cam and math.abs(s.cam) > 1e-3 then cam = true end
  end
  report("run", "core", { audio = Tel.counts.audio > a0, ui = Tel.counts.ui > u0, anim = false, camera = cam,
                          particles = false, n = (Tel.counts.audio > a0 and 1 or 0) + (cam and 1 or 0) })
  -- jump and landing
  settle(t, FIELD[1], FIELD[2], FIELD[3])
  t._hold("jump", 1)
  report("jump", "core", channels(t, 10))
  local s, base
  repeat base = counts() s = step(t) until s.ground and s.y < FIELD[2] + 0.01
  report("land", "core", channels(t, 10, nil, s, base))
  -- a jump from the 1 m ledge lands harder
  ledge(t)
  repeat run_on() s = step(t) until not s.ground
  repeat base = counts() s = step(t) until s.ground
  report("land_from_ledge", "high", channels(t, 10, nil, s, base))
  -- the keep door
  t.place("keep_door")
  t.face("keep.door", 1.1, 1.6, 0)
  t.ticks(2)
  Tel.ui_new()
  t._hold("use", 1)
  report("door", "core", channels(t, 15, { "keep.door" }))
  t.ticks(40)
  require("props.door").toggle(prts.entity.find("keep.door"))
  t.ticks(40)
  -- talking to the guard at the keep door
  t.place(2.3, 0, 13.2)
  t.face("npc.door_guard", 0, 1.3, 0)
  t.ticks(2)
  Tel.ui_new()
  t._hold("use", 1)
  report("talk", "core", channels(t, 15, { "npc.door_guard" }))
  t.ticks(300)
  -- the hoist lift's lever, and the lift arriving
  t.place("keep_hall_lift")
  t.ticks(2)
  t._hold("move_right", 36)
  t.ticks(36)
  t.look(math.pi / 2, 0)
  t._hold("move_forward", 72)
  t.ticks(78)
  t.face("keep.lift.lever")
  t.ticks(2)
  Tel.ui_new()
  t._hold("use", 1)
  report("lift_lever", "core", channels(t, 15, { "keep.lift.platform" }))
  local platform = prts.entity.find("keep.lift.platform")
  repeat base = counts() s = step(t) until not prts.entity.moving(platform)
  report("lift_arrives", "high", channels(t, 10, nil, s, base))
  -- the King's Gate portcullis: the winch, and the grille slamming down
  local map = require("maps.proving_ground")
  local grille = map.kings_gate
  local winch = prts.entity.data(grille).winch
  local wx, _, wz = prts.entity.pos(winch)
  t.place(wx - 1.0, 0, wz - 0.4)
  t.face(prts.entity.name(winch))
  t.ticks(2)
  Tel.ui_new()
  t._hold("use", 1)
  report("winch", "core", channels(t, 15, { prts.entity.name(grille) }))
  repeat base = counts() s = step(t) until not prts.entity.moving(grille)
  report("portcullis_slams", "high", channels(t, 10, nil, s, base))
  require("props.portcullis").toggle(grille)
  t.ticks(200)
  return c
end

-- The use ray's reach around a usable thing, from 1.6 m in front of it at eye height: the
-- half-widths (m, at the thing) left/right and up/down where it is still picked.
function P.reach(t, name, ox, oy, oz, from)
  t.place(from[1], from[2], from[3])
  t.ticks(2)
  t.face(name, ox, oy, oz)
  t.ticks(1)
  local yaw0, pitch0 = prts.commander.view()
  local e = prts.entity.find(name)
  local ex, ey, ez = prts.entity.pos(e)
  local _, _, eye = prts.commander.view()
  local cx, cy, cz = t.pos(t.commander())
  local dist = math.sqrt((ex + (ox or 0) - cx) ^ 2 + (ez + (oz or 0) - cz) ^ 2) -- to the point looked at
  local function extent(dyaw, dpitch)
    local a = 0
    for k = 1, 400 do
      local ang = k * math.rad(0.25)
      t.look(yaw0 + dyaw * ang, pitch0 + dpitch * ang)
      t.ticks(1)
      if t.target() ~= name then break end
      a = ang
    end
    return a
  end
  local left, right = extent(1, 0), extent(-1, 0)
  local up, down = extent(0, 1), extent(0, -1)
  t.look(yaw0, pitch0)
  local out = { name = name, dist = dist, eye = eye, ey = ey, cy = cy, left = left, right = right, up = up, down = down,
                w = dist * (math.tan(left) + math.tan(right)), h = dist * (math.tan(up) + math.tan(down)) }
  Tel.emit("probe_reach", out)
  return out
end

function P.all(t, trials)
  Tel.install(t)
  P.latency(t)
  P.motion(t)
  P.timing(t, trials)
  P.feedback(t)
  local map = require("maps.proving_ground")
  local wx, _, wz = prts.entity.pos(prts.entity.data(map.kings_gate).winch)
  P.reach(t, "keep.door", 1.1, 1.6, 0, { 0, 0, 11.6 })
  P.reach(t, "npc.door_guard", 0, 0.92, 0, { 2.3, 0, 13.0 })
  P.reach(t, "npc.hall_guard", 0, 0.92, 0, { 2.6, 0.2, -6.5 })
  P.reach(t, prts.entity.name(prts.entity.data(map.kings_gate).winch), 0, 0, 0, { wx - 1.2, 0, wz - 0.4 })
  -- the call button on the second floor (the great hall's patrol walks past the one below)
  local cx, _, cz = prts.entity.pos(prts.entity.find("keep.lift.call1"))
  P.reach(t, "keep.lift.call1", 0, 0, 0, { cx + 1.4, map.FLOORS[2], cz })
  Tel.emit("probes_done", {})
end

return P
