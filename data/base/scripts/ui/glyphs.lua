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

local function touch_button(code)
  if code == 0 then return "Tap" end
  local b = prts.input.touch_state().buttons[code]
  if not b then return "Button " .. code end
  return b.id:sub(1, 1):upper() .. b.id:sub(2) .. " button"
end

-- The name of one binding as returned by prts.input.bindings.
function G.name(b)
  if b.kind == "key" then return b.name end
  if b.kind == "mouse_button" then return G.MOUSE[b.code] or ("Mouse " .. b.code) end
  if b.kind == "mouse_axis" then return G.MOUSE_AXES[b.code] end
  if b.kind == "pad_button" then return G.PAD[b.code] or ("Button " .. b.code) end
  if b.kind == "pad_axis" then return G.PAD_AXES[b.code] end
  if b.kind == "touch_axis" then return G.TOUCH_AXES[b.code] end
  return touch_button(b.code)
end

-- All bindings of an action in one scheme, e.g. "E, Right mouse"; "none" if unbound.
function G.list(action, scheme)
  local names = {}
  for _, b in ipairs(prts.input.bindings(action)[scheme] or {}) do names[#names + 1] = G.name(b) end
  return #names > 0 and table.concat(names, ", ") or "none"
end

-- The glyph that starts an interaction prompt: [E] on keyboard, (X) on a gamepad, Tap: on touch.
function G.prompt(action, scheme)
  local first = (prts.input.bindings(action)[scheme] or {})[1]
  if not first then return "[?]" end
  if scheme == "gamepad" then return "(" .. G.name(first) .. ")" end
  if scheme == "touch" then return G.name(first) .. ":" end
  return "[" .. G.name(first) .. "]"
end

return G
