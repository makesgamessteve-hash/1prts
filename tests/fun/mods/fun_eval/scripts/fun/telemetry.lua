-- Telemetry for the fun audit: a JSONL stream on the log ("FUN {...}" lines) that
-- tools/fun/audit.py reads. It hooks the game without changing it: sounds started
-- (prts.audio.play), subtitles shown, entities used, text drawn by the UI, and a state sample
-- of the commander every tick (kept for delayed perception, logged every few ticks).
-- Everything is stamped with the simulation tick; nothing reads the wall clock. MIT License.
local json = require("fun.json")

local T = { every = 3, history = {}, counts = { audio = 0, ui = 0, use = 0, camera = 0, anim = 0 } }
T.SAMPLE_KEEP = 90 -- ticks of state kept for perception delays

function T.emit(kind, fields)
  fields = fields or {}
  fields.k = kind
  fields.t = prts.time.tick()
  prts.log.info("FUN " .. json.encode(fields))
end

local installed = false

-- Hooks the game's feedback channels. Safe to call more than once.
function T.install(t)
  T.t = t
  if installed then return T end
  installed = true
  local play = prts.audio.play
  prts.audio.play = function(opts)
    T.counts.audio = T.counts.audio + 1
    T.emit("audio", { sound = type(opts) == "table" and opts.sound or "?" })
    return play(opts)
  end
  local subtitles = require("ui.subtitles")
  for _, name in ipairs({ "say", "interrupt" }) do
    local f = subtitles[name]
    subtitles[name] = function(speaker, text, seconds)
      T.counts.ui = T.counts.ui + 1
      T.emit("subtitle", { speaker = speaker, len = #text })
      return f(speaker, text, seconds)
    end
  end
  prts.events.on("use", "fun.telemetry", function(e)
    T.counts.use = T.counts.use + 1
    local data = prts.entity.data(e)
    T.emit("use", { name = prts.entity.name(e), kind = data.kind or "" })
    return false
  end)
  T.ui_seen = {}
  return T
end

-- New text drawn on the screen since the last call (subtitles, prompts, notices, toasts).
function T.ui_new()
  local t = T.t
  if not t.ui_texts then return {} end
  local now, fresh = {}, {}
  for _, s in ipairs(t.ui_texts()) do
    now[s] = true
    if not T.ui_seen[s] then fresh[#fresh + 1] = s end
  end
  T.ui_seen = now
  return fresh
end

-- The commander's state this tick; call once per tick. Returns the sample.
function T.sample()
  local t = T.t
  local c = t.commander()
  local x, y, z = t.pos(c)
  local yaw, pitch, eye, ground = prts.commander.view()
  local s = { tick = prts.time.tick(), x = x, y = y, z = z, yaw = yaw, pitch = pitch, eye = eye, ground = ground }
  local feel = prts.commander.feel and prts.commander.feel() or nil
  if feel then s.cam, s.camp = feel.offset, feel.pitch end
  local h = T.history
  local prev = h[#h]
  if prev then
    local dt = prts.time.dt()
    s.vx, s.vy, s.vz = (x - prev.x) / dt, (y - prev.y) / dt, (z - prev.z) / dt
  else
    s.vx, s.vy, s.vz = 0, 0, 0
  end
  h[#h + 1] = s
  if #h > T.SAMPLE_KEEP then table.remove(h, 1) end
  local fresh = T.ui_new()
  if #fresh > 0 then
    T.counts.ui = T.counts.ui + #fresh
    local texts = {}
    for i, text in ipairs(fresh) do texts[i] = text:sub(1, 48) end
    T.emit("ui", { n = #fresh, texts = texts })
  end
  if s.tick % T.every == 0 then
    T.emit("s", { x = x, y = y, z = z, yaw = yaw, ground = ground, vx = s.vx, vy = s.vy, vz = s.vz, cam = s.cam,
                  camp = s.camp })
  end
  return s
end

-- The sample `delay` ticks ago (the oldest kept one if there is not that much history).
function T.ago(delay)
  local h = T.history
  return h[math.max(1, #h - delay)]
end

return T
