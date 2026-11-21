-- Names of keys, buttons and touch controls, for prompts and the controls menu. MIT License.
local G = {}

-- Gamepad buttons in SDL order (Xbox-style face button names).
G.PAD = {
  [0] = "A", [1] = "B", [2] = "X", [3] = "Y", [4] = "Back", [5] = "Guide", [6] = "Start", [7] = "LS",
  [8] = "RS", [9] = "LB", [10] = "RB", [11] = "D-pad up", [12] = "D-pad down", [13] = "D-pad left",
  [14] = "D-pad right",
}
G.PAD_AXES = { [0] = "Left stick", [1] = "Left stick", [2] = "Right stick", [3] = "Right stick", [4] = "LT", [5] = "RT" }
G.MOUSE = { [1] = "Left mouse", [2] = "Middle mouse", [3] = "Right mouse", [4] = "Mouse 4", [5] = "Mouse 5" }
G.MOUSE_AXES = { [0] = "Mouse", [1] = "Mouse", [2] = "Wheel" }
G.TOUCH_AXES = { [0] = "Stick", [1] = "Stick", [2] = "Drag", [3] = "Drag" }
-- XR controllers (xr-standard order): codes 0-7 left hand, 8-15 right; axes 0-3 left, 4-7 right.
G.XR = { [0] = "trigger", [1] = "grip", [2] = "touchpad", [3] = "stick press", [4] = "X", [5] = "Y" }
G.XR_RIGHT = { [4] = "A", [5] = "B" }
G.XR_AXES = { [0] = "touchpad", [1] = "touchpad", [2] = "stick", [3] = "stick" }

local function xr_name(code, per_hand, names)
  local hand, i = code // per_hand, code % per_hand
  local name = hand == 1 and G.XR_RIGHT[i] or names[i]
  if not name then return (hand == 1 and "Right" or "Left") .. " button " .. i end
  if #name == 1 then return name end
  return (hand == 1 and "Right " or "Left ") .. name
end

local function touch_button(code)
  if code == 0 then return "Tap" end
  local b = prts.input.touch_state().buttons[code]
  if not b then return "Button " .. code end
  local label = b.id
  for _, l in ipairs(require("ui.touch").LAYOUT.buttons) do
    if l.id == b.id then label = l.label or label end
  end
  return label:sub(1, 1):upper() .. label:sub(2) .. " button"
end

-- The name of one binding as returned by prts.input.bindings.
function G.name(b)
  if b.kind == "key" then return b.name end
  if b.kind == "mouse_button" then return G.MOUSE[b.code] or ("Mouse " .. b.code) end
  if b.kind == "mouse_axis" then return G.MOUSE_AXES[b.code] end
  if b.kind == "pad_button" then return G.PAD[b.code] or ("Button " .. b.code) end
  if b.kind == "pad_axis" then return G.PAD_AXES[b.code] end
  if b.kind == "touch_axis" then return G.TOUCH_AXES[b.code] end
  if b.kind == "xr_button" then return xr_name(b.code, 8, G.XR) end
  if b.kind == "xr_axis" then return xr_name(b.code, 4, G.XR_AXES) end
  return touch_button(b.code)
end

-- All bindings of an action in one scheme, e.g. "E, Right mouse"; "none" if unbound.
function G.list(action, scheme)
  local names = {}
  for _, b in ipairs(prts.input.bindings(action)[scheme] or {}) do names[#names + 1] = G.name(b) end
  return #names > 0 and table.concat(names, ", ") or "none"
end

-- The glyph that starts an interaction prompt: [E] on keyboard, (X) on a gamepad or a VR
-- controller, Tap: on touch.
function G.prompt(action, scheme)
  local first = (prts.input.bindings(action)[scheme] or {})[1]
  if not first then return "[?]" end
  if scheme == "gamepad" or scheme == "xr" then return "(" .. G.name(first) .. ")" end
  if scheme == "touch" then return G.name(first) .. ":" end
  return "[" .. G.name(first) .. "]"
end

return G
