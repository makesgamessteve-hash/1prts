-- Saddled horses the commander mounts and rides (First-Person Contract: from the saddle, with the
-- horse's neck and head in view). A horse standing about is an entity with a horse sprite; while
-- the commander rides it, it exists only as the rider's state and as the head and neck in front
-- of the eyes. Dismounting leaves it standing beside the commander. MIT License.
local H = {}
H.STATE = "commander.mount" -- an entity whose data is the mount state (saved and hashed)
H.HEAD = "commander.horse_head"
H.REACH = 3.0               -- the mount action mounts a horse this close
-- Trot, canter and gallop; the eye at saddle height.
H.RIDING = { walk_speed = 3.0, run_speed = 7.5, sprint_speed = 12.0, crouch_speed = 0, eye_stand = 2.55,
             step_height = 0.5 }
H.KEEP = { "walk_speed", "run_speed", "sprint_speed", "crouch_speed", "eye_stand", "step_height" }
H.SOUND = "sounds/footstep.wav"

-- The horse's coat for the head in view (maps call this with their materials).
function H.materials()
  prts.scene.material{ id = "horse_coat", texture = "textures/wood_dark.png", tint = { 1.05, 0.72, 0.48 }, uv_scale = 4 }
  prts.scene.material{ id = "horse_mane", texture = "textures/iron.png", tint = { 0.25, 0.2, 0.16 }, uv_scale = 2 }
end

-- A saddled horse standing at (x, y, z) facing yaw.
function H.make(cfg)
  local e = prts.entity.spawn{ name = cfg.name, pos = cfg.pos, yaw = cfg.yaw or 0, usable = true }
  prts.scene.box{ min = { -0.5, 0, -1.2 }, max = { 0.5, 2.0, 1.2 }, entity = e, visible = false, use_only = true }
  local d = prts.entity.data(e)
  d.kind = "horse"
  d.sprite = prts.scene.sprite{ sprite = "sprites/horse.lua", entity = e, anim = "idle" }
  return e
end

local function state()
  local e = prts.entity.find(H.STATE)
  if not e then e = prts.entity.spawn{ name = H.STATE } end
  return prts.entity.data(e)
end

function H.mounted()
  local e = prts.entity.find(H.STATE)
  return e ~= nil and prts.entity.data(e).riding == true
end

-- The neck, mane, head and ears ahead of and below the rider's eyes, turned with the view.
local function show_head()
  local c = prts.commander.entity()
  local yaw = prts.commander.view()
  local e = prts.entity.spawn{ name = H.HEAD, parent = c, pos = { 0, 0, 0 }, yaw = yaw - prts.entity.yaw(c) }
  local function b(mat, x0, y0, z0, x1, y1, z1)
    prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = false }
  end
  b("horse_coat", -0.13, 1.45, -1.05, 0.13, 2.12, -0.45)   -- the neck
  b("horse_mane", -0.04, 2.12, -1.0, 0.04, 2.2, -0.45)     -- the mane
  b("horse_coat", -0.12, 1.78, -1.62, 0.12, 2.08, -1.0)    -- the head, reaching forward
  b("horse_coat", -0.12, 2.08, -1.08, -0.05, 2.24, -1.0)   -- the ears
  b("horse_coat", 0.05, 2.08, -1.08, 0.12, 2.24, -1.0)
  return e
end

-- Mounts the horse entity `horse`. Returns true.
function H.mount(horse)
  local st = state()
  if st.riding or not prts.entity.alive(horse) or prts.entity.data(horse).kind ~= "horse" then return false end
  local now = prts.commander.tune()
  local keep = {}
  for _, k in ipairs(H.KEEP) do keep[k] = now[k] end
  prts.commander.tune(H.RIDING)
  st.saved, st.riding, st.horse = keep, true, prts.entity.name(horse)
  prts.entity.despawn(horse)
  show_head()
  prts.events.emit("mounted", st.horse)
  return true
end

-- Dismounts: the commander's own speeds come back and the horse stands beside them.
function H.dismount()
  local st = state()
  if not st.riding then return nil end
  prts.commander.tune(st.saved)
  local head = prts.entity.find(H.HEAD)
  if head then prts.entity.despawn(head) end
  local c = prts.commander.entity()
  local x, y, z = prts.entity.pos(c)
  local yaw = prts.commander.view()
  local rx, rz = math.cos(yaw), -math.sin(yaw) -- the rider's right
  local horse = H.make{ name = st.horse, pos = { x + rx * 1.6, y, z + rz * 1.6 }, yaw = yaw }
  st.riding, st.saved = false, nil
  prts.events.emit("dismounted", st.horse)
  return horse
end

-- The nearest standing horse within H.REACH of the commander, or nil.
function H.nearest()
  local c = prts.commander.entity()
  if not c then return nil end
  local x, _, z = prts.entity.pos(c)
  local best, bd = nil, H.REACH
  for i = 1, 16 do
    local e = prts.entity.find("horse." .. i)
    if e then
      local hx, _, hz = prts.entity.pos(e)
      local dist = math.sqrt((hx - x) ^ 2 + (hz - z) ^ 2)
      if dist <= bd then best, bd = e, dist end
    end
  end
  return best
end

-- The mount action: dismount when riding, else mount the nearest horse.
function H.toggle()
  if H.mounted() then return H.dismount() ~= nil end
  local horse = H.nearest()
  return horse ~= nil and H.mount(horse)
end

-- Each tick: the mount action (pressed, not held), and the head turned with the view.
function H.tick()
  if not prts.commander.entity() then return end
  local down = prts.input.down("mount")
  local se = prts.entity.find(H.STATE)
  if not down and not se then return end -- nothing to do (and no state entity on maps without horses)
  local st = state()
  if st.pending then
    local horse = prts.entity.find(st.pending)
    st.pending = nil
    if horse then H.mount(horse) end
  end
  if down and not st.prev then H.toggle() end
  st.prev = down or nil
  local head = st.riding and prts.entity.find(H.HEAD)
  if head then prts.entity.place(head, { 0, 0, 0 }, prts.commander.view() - prts.entity.yaw(prts.commander.entity())) end
end

-- Using a horse mounts it on the next tick: other handlers of this use still see the horse.
function H.on_use(e)
  if prts.entity.data(e).kind ~= "horse" then return false end
  state().pending = prts.entity.name(e)
  return true
end

-- the touch button shows while riding or beside a horse
require("ui.touch").can_ride = function() return H.mounted() or H.nearest() ~= nil end

prts.events.on("use", "props.horse", H.on_use)
prts.events.on("tick", "props.horse", H.tick)
return H
