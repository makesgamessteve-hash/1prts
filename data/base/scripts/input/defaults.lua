-- Default actions and bindings. Players can rebind them; mods can add more. MIT License.
local M = {}

-- Gamepad axes: 0 left x, 1 left y, 2 right x, 3 right y. Buttons: 0 south, 1 east, 2 west.
-- Touch axes: 0, 1 the movement stick (x, y), 2, 3 the look drag. Touch buttons: 0 a tap on
-- the look side, 1.. the context buttons of the touch layout (ui/touch.lua).
-- XR controllers (WebXR xr-standard order), left hand first: buttons 0-7 left, 8-15 right
-- (+0 trigger, +1 squeeze, +3 thumbstick press, +4 A/X, +5 B/Y); axes 0-3 left, 4-7 right
-- (+2 thumbstick x, +3 thumbstick y, -1 when pushed forward). In XR the head looks around,
-- the right stick turns (look_x) and the left stick walks, or aims a teleport when teleport
-- movement is chosen (game/settings.lua).
M.XR = { LEFT_TRIGGER = 0, LEFT_STICK_PRESS = 3, LEFT_Y = 5, RIGHT_TRIGGER = 8, RIGHT_A = 12, RIGHT_B = 13,
         LEFT_STICK_X = 2, LEFT_STICK_Y = 3, RIGHT_STICK_X = 6 }
local XR = M.XR

M.actions = {
  { id = "move_forward", label = "Move forward", keys = { "W", "Up" }, pad_axes = { { code = 1, scale = -1 } },
    touch_axes = { { code = 1, scale = -1 } }, xr_axes = { { code = XR.LEFT_STICK_Y, scale = -1 } } },
  { id = "move_back", label = "Move back", keys = { "S", "Down" }, pad_axes = { { code = 1, scale = 1 } },
    xr_axes = { { code = XR.LEFT_STICK_Y, scale = 1 } } },
  { id = "move_left", label = "Move left", keys = { "A", "Left" }, pad_axes = { { code = 0, scale = -1 } },
    xr_axes = { { code = XR.LEFT_STICK_X, scale = -1 } } },
  { id = "move_right", label = "Move right", keys = { "D", "Right" }, pad_axes = { { code = 0, scale = 1 } },
    touch_axes = { { code = 0, scale = 1 } }, xr_axes = { { code = XR.LEFT_STICK_X, scale = 1 } } },
  { id = "look_x", label = "Look left/right", mouse_axes = { 0 }, pad_axes = { { code = 2, scale = 12 } },
    touch_axes = { { code = 2, scale = 1 } }, xr_axes = { { code = XR.RIGHT_STICK_X, scale = 1 } } },
  { id = "look_y", label = "Look up/down", mouse_axes = { 1 }, pad_axes = { { code = 3, scale = 12 } },
    touch_axes = { { code = 3, scale = 1 } } },
  { id = "run", label = "Run", keys = { "Left Shift" }, pad_buttons = { 7 }, touch_buttons = { 4 },
    xr_buttons = { XR.LEFT_STICK_PRESS } },
  { id = "sprint", label = "Sprint", keys = { "Left Alt" }, pad_buttons = { 8 } },
  { id = "crouch", label = "Crouch", keys = { "Left Ctrl", "C" }, pad_buttons = { 1 }, touch_buttons = { 3 },
    xr_buttons = { XR.RIGHT_B } },
  { id = "jump", label = "Jump", keys = { "Space" }, pad_buttons = { 0 }, touch_buttons = { 2 },
    xr_buttons = { XR.RIGHT_A } },
  { id = "use", label = "Use", keys = { "E" }, mouse_buttons = { 3 }, pad_buttons = { 2 }, touch_buttons = { 0, 1 },
    xr_buttons = { XR.RIGHT_TRIGGER, XR.LEFT_TRIGGER } },
  -- The commander's tools in a skirmish (rts/tools.lua): the field map, the sending stone, the
  -- marking staff, the war horn and picking the next company.
  { id = "field_map", label = "Field map", keys = { "Tab", "M" }, pad_buttons = { 4 }, touch_buttons = { 5 },
    xr_buttons = { XR.LEFT_Y } },
  { id = "stone", label = "Sending stone", keys = { "Q" }, pad_buttons = { 3 }, touch_buttons = { 6 } },
  { id = "mark", label = "Marking staff", keys = { "F" }, mouse_buttons = { 2 }, pad_buttons = { 10 },
    touch_buttons = { 7 } },
  { id = "horn", label = "War horn", keys = { "H" }, pad_buttons = { 9 }, touch_buttons = { 8 } },
  { id = "next_company", label = "Next company", keys = { "X" }, pad_buttons = { 14 }, touch_buttons = { 9 } },
  -- Teleport movement (XR): push the stick forward to aim with that hand's ray, let go to go.
  { id = "teleport", label = "Teleport (VR)", xr_axes = { { code = XR.LEFT_STICK_Y, scale = -1 } } },
}

function M.apply()
  for _, action in ipairs(M.actions) do
    prts.input.action(action)
  end
  return #M.actions
end

return M
