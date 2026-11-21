-- The hoist lift: a platform that rides between floors, a lever on the platform, call
-- levers on every floor and a sliding gate per floor. MIT License.
local Lift = {}
local SPEED = 1.6      -- m/s
local GATE_RISE = 2.6  -- how far a gate slides up when open
local GATE_SPEED = 2.0

-- cfg: name, pos {x, y, z} of the platform at the ground floor, floors {heights},
-- size {w, d}, gate (x offset of the gates), material, gate_material, lever_material
function Lift.make(cfg)
  local x, y0, z = cfg.pos[1], cfg.pos[2], cfg.pos[3]
  local hw, hd = cfg.size[1] * 0.5, cfg.size[2] * 0.5
  local platform = prts.entity.spawn{ name = cfg.name .. ".platform", pos = { x, y0, z } }
  prts.scene.box{ min = { -hw, -0.25, -hd }, max = { hw, 0, hd }, material = cfg.material, entity = platform, tile = 1.0 }
  local lever = prts.entity.spawn{ name = cfg.name .. ".lever", parent = platform, usable = true,
                                   pos = { -hw + 0.2, 1.3, hd - 0.3 } }
  prts.scene.box{ min = { -0.08, -0.25, -0.08 }, max = { 0.08, 0.25, 0.08 }, material = cfg.lever_material, entity = lever }
  local state = prts.entity.data(platform)
  state.kind = "lift"
  state.floors = cfg.floors
  state.current = 1
  state.target = 1
  state.x, state.z = x, z
  state.gates = {}
  state.calls = {}
  state.lever = lever
  for i, h in ipairs(cfg.floors) do
    local gate = prts.entity.spawn{ name = cfg.name .. ".gate" .. (i - 1), pos = { cfg.gate, h, z } }
    prts.scene.box{ min = { -0.06, 0, -1.2 }, max = { 0.06, 2.8, 1.2 }, material = cfg.gate_material, entity = gate, tile = 1.0 }
    state.gates[i] = gate
    local call = prts.entity.spawn{ name = cfg.name .. ".call" .. (i - 1), usable = true,
                                    pos = { cfg.gate + 0.35, h + 1.35, z + 1.65 } }
    prts.scene.box{ min = { -0.08, -0.25, -0.08 }, max = { 0.08, 0.25, 0.08 }, material = cfg.lever_material, entity = call }
    state.calls[i] = call
    prts.entity.data(call).kind = "lift_call"
    prts.entity.data(call).lift = platform
    prts.entity.data(call).floor = i
    prts.entity.data(gate).open = false
  end
  prts.entity.data(lever).kind = "lift_lever"
  prts.entity.data(lever).lift = platform
  Lift.set_gate(platform, 1, true)
  return platform
end

function Lift.set_gate(platform, i, open)
  local state = prts.entity.data(platform)
  local gate = state.gates[i]
  local gdata = prts.entity.data(gate)
  if gdata.open == open then return false end
  gdata.open = open
  local gx, _, gz = prts.entity.local_pos(gate)
  prts.entity.move_to(gate, { gx, state.floors[i] + (open and GATE_RISE or 0), gz }, GATE_SPEED)
  return true
end

function Lift.go_to(platform, i)
  local state = prts.entity.data(platform)
  if prts.entity.moving(platform) or i == state.current then
    Lift.set_gate(platform, state.current, true)
    return false
  end
  for g = 1, #state.gates do Lift.set_gate(platform, g, false) end
  state.target = i
  prts.entity.move_to(platform, { state.x, state.floors[i], state.z }, SPEED)
  return true
end

-- The platform lever goes to the top floor, or back down when already at the top.
function Lift.lever_target(state)
  local top = #state.floors
  return state.current == top and 1 or top
end

function Lift.on_use(e)
  local data = prts.entity.data(e)
  if data.kind == "lift_call" then
    return Lift.go_to(data.lift, data.floor)
  elseif data.kind == "lift_lever" then
    return Lift.go_to(data.lift, Lift.lever_target(prts.entity.data(data.lift)))
  end
  return false
end

function Lift.on_move_done(e)
  local state = prts.entity.data(e)
  if state.kind ~= "lift" then return false end
  state.current = state.target
  Lift.set_gate(e, state.current, true)
  return true
end

prts.events.on("use", "props.lift", Lift.on_use)
prts.events.on("move_done", "props.lift", Lift.on_move_done)
return Lift
