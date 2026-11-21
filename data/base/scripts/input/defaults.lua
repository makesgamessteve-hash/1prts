-- Default actions and bindings. Players can rebind them; mods can add more. MIT License.
local M = {}

-- Gamepad axes: 0 left x, 1 left y, 2 right x, 3 right y. Buttons: 0 south, 1 east, 2 west.
-- Touch axes: 0, 1 the movement stick (x, y), 2, 3 the look drag. Touch buttons: 0 a tap on
-- the look side, 1.. the context buttons of the touch layout (ui/touch.lua).
M.actions = {
  { id = "move_forward", label = "Move forward", keys = { "W", "Up" }, pad_axes = { { code = 1, scale = -1 } },
    touch_axes = { { code = 1, scale = -1 } } },
  { id = "move_back", label = "Move back", keys = { "S", "Down" }, pad_axes = { { code = 1, scale = 1 } } },
  { id = "move_left", label = "Move left", keys = { "A", "Left" }, pad_axes = { { code = 0, scale = -1 } } },
  { id = "move_right", label = "Move right", keys = { "D", "Right" }, pad_axes = { { code = 0, scale = 1 } },
    touch_axes = { { code = 0, scale = 1 } } },
  { id = "look_x", label = "Look left/right", mouse_axes = { 0 }, pad_axes = { { code = 2, scale = 12 } },
    touch_axes = { { code = 2, scale = 1 } } },
  { id = "look_y", label = "Look up/down", mouse_axes = { 1 }, pad_axes = { { code = 3, scale = 12 } },
    touch_axes = { { code = 3, scale = 1 } } },
  { id = "run", label = "Run", keys = { "Left Shift" }, pad_buttons = { 7 }, touch_buttons = { 4 } },
  { id = "sprint", label = "Sprint", keys = { "Left Alt" }, pad_buttons = { 8 } },
  { id = "crouch", label = "Crouch", keys = { "Left Ctrl", "C" }, pad_buttons = { 1 }, touch_buttons = { 3 } },
  { id = "jump", label = "Jump", keys = { "Space" }, pad_buttons = { 0 }, touch_buttons = { 2 } },
  { id = "use", label = "Use", keys = { "E" }, mouse_buttons = { 3 }, pad_buttons = { 2 }, touch_buttons = { 0, 1 } },
}

function M.apply()
  for _, action in ipairs(M.actions) do
    prts.input.action(action)
  end
  return #M.actions
end

return M
