-- The diving bell (M8): a bronze bell open at the bottom, hanging from a crane on a jetty. Step
-- in through its open side and pull the lever: it goes down on its chain to the seabed (or back
-- up) with whoever stands in it. Its inside is full of air, and a small table of shell and sand
-- shows the war: a command post under the water. MIT License.
local Bell = {}
Bell.SPEED = 1.2
Bell.TABLE = { 0, 0, -1.0 }

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- cfg = { name, x, z, yaw, top }: the bell hangs from a crane over (x, z) with its floor at `top`
-- (the jetty's height); it goes down to the seabed under it.
function Bell.make(cfg)
  local floor = prts.scene.height(cfg.x, cfg.z) + 0.3
  local top = cfg.top or (require("rts.water").level() or 0) + 1.2
  local crane = prts.entity.spawn{ name = cfg.name .. ".crane", pos = { cfg.x, top, cfg.z }, yaw = cfg.yaw or 0 }
  box(crane, "oak", 3.6, -top + floor - 1, -0.3, 4.2, 8, 0.3)  -- the crane's post and jib
  box(crane, "oak", -0.3, 7.6, -0.3, 4.2, 8, 0.3, false)
  local e = prts.entity.spawn{ name = cfg.name, pos = { cfg.x, top, cfg.z }, yaw = cfg.yaw or 0 }
  box(e, "planks", -2.4, -0.2, -2.4, 2.4, 0, 2.4)              -- the floor grating
  box(e, "iron", -2.6, 0, -2.6, -2.4, 2.8, 2.6)                 -- the bell's walls, open aft (+z)
  box(e, "iron", 2.4, 0, -2.6, 2.6, 2.8, 2.6)
  box(e, "iron", -2.6, 0, -2.6, 2.6, 2.8, -2.4)
  box(e, "iron", -2.6, 2.8, -2.6, 2.6, 3.2, 2.6)                -- the crown
  box(e, "iron", -0.05, 3.0, -0.05, 0.05, 8, 0.05, false)       -- the chain
  prts.scene.light{ pos = { 0, 2.2, 0 }, color = { 1.0, 0.85, 0.6 }, radius = 6, entity = e }
  local lever = prts.entity.spawn{ name = cfg.name .. ".lever", parent = e, pos = { 2.0, 1.3, 1.2 }, usable = true }
  box(lever, "oak", -0.06, -0.3, -0.06, 0.06, 0.3, 0.06)
  prts.scene.box{ min = { -0.15, -0.35, -0.15 }, max = { 0.15, 0.35, 0.15 }, entity = lever, visible = false, use_only = true }
  local ld = prts.entity.data(lever)
  ld.kind, ld.bell, ld.prompt = "bell_lever", cfg.name, "Work the bell's chain"
  require("rts.water").air(e, { -2.4, -0.2, -2.4 }, { 2.4, 2.8, 2.6 })
  local d = prts.entity.data(e)
  d.kind, d.top, d.bottom, d.down = "bell", top, floor, false
  return e
end

-- Sends the bell down, or up if it is down (or going down).
function Bell.toggle(e)
  local d = prts.entity.data(e)
  d.down = not d.down
  local x, _, z = prts.entity.local_pos(e)
  prts.entity.move_to(e, { x, d.down and d.bottom or d.top, z }, Bell.SPEED)
  if prts.audio then prts.audio.play{ sound = "sounds/lift_clank.wav", entity = e } end
  prts.events.emit("bell", prts.entity.name(e), d.down)
  return d.down
end

-- Whether the bell rests on the seabed.
function Bell.at_bottom(e)
  local d = prts.entity.data(e)
  local _, y = prts.entity.local_pos(e)
  return d.down and math.abs(y - d.bottom) < 0.05
end

-- The bell's shell table (for the commander's side).
function Bell.furnish(e)
  return require("rts.tools").furnish(e, Bell.TABLE)
end

prts.events.on("use", "props.bell", function(lever)
  local d = prts.entity.data(lever)
  if d.kind ~= "bell_lever" then return false end
  local e = prts.entity.find(d.bell)
  return e and Bell.toggle(e)
end)
return Bell
