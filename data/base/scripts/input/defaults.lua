-- Default actions and bindings. Players can rebind them; mods can add more. MIT License.
local M = {}

-- Gamepad axes: 0 left x, 1 left y, 2 right x, 3 right y. Buttons: 0 south, 1 east, 2 west.
M.actions = {
  { id = "move_forward", keys = { "W", "Up" },    pad_axes = { { code = 1, scale = -1 } } },
  { id = "move_back",    keys = { "S", "Down" },  pad_axes = { { code = 1, scale = 1 } } },
  { id = "move_left",    keys = { "A", "Left" },  pad_axes = { { code = 0, scale = -1 } } },
  { id = "move_right",   keys = { "D", "Right" }, pad_axes = { { code = 0, scale = 1 } } },
  { id = "look_x",       mouse_axes = { 0 },      pad_axes = { { code = 2, scale = 12 } } },
  { id = "look_y",       mouse_axes = { 1 },      pad_axes = { { code = 3, scale = 12 } } },
  { id = "run",          keys = { "Left Shift" }, pad_buttons = { 7 } },
  { id = "sprint",       keys = { "Left Alt" },   pad_buttons = { 8 } },
  { id = "crouch",       keys = { "Left Ctrl", "C" }, pad_buttons = { 1 } },
  { id = "jump",         keys = { "Space" },      pad_buttons = { 0 } },
  { id = "use",          keys = { "E" },          mouse_buttons = { 3 }, pad_buttons = { 2 } },
}

function M.apply()
  for _, action in ipairs(M.actions) do
    prts.input.action(action)
  end
  return #M.actions
end

return M
