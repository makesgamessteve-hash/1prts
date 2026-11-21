-- The controls page of the options menu: every action with its bindings per input scheme.
-- Keyboard/mouse, gamepad and VR controller bindings change by pressing the new key or button; touch
-- bindings cycle through the touch controls. Changes are saved at once. MIT License.
local glyphs = require("ui.glyphs")

local C = { scheme = "kbm", capturing = nil, capture_label = nil }

C.SCHEMES = {
  { id = "kbm", label = "Keyboard & mouse" },
  { id = "gamepad", label = "Gamepad" },
  { id = "touch", label = "Touch" },
  { id = "xr", label = "VR" },
}

-- Stops waiting for a key; returns whether it was waiting.
function C.cancel()
  local was = C.capturing ~= nil
  C.capturing = nil
  return was
end

-- The next touch control for an action: none, the tap, then each context button.
function C.next_touch(action)
  local code = -1
  for _, b in ipairs(prts.input.bindings(action).touch) do
    if b.kind == "touch_button" then code = b.code break end
  end
  code = code + 1
  if code > #prts.input.touch_state().buttons then return {} end
  return { touch_buttons = { code } }
end

function C.change(action, label)
  if C.scheme == "touch" then
    prts.input.rebind(action, "touch", C.next_touch(action))
    prts.settings.save()
  else
    C.capturing, C.capture_label = action, label
  end
end

-- While waiting, the next key or button of the scheme becomes the binding (Esc cancels).
function C.poll()
  if not C.capturing then return false end
  local binding = prts.input.capture(C.scheme)
  if not binding then return false end
  if binding.keys and binding.keys[1] == "Escape" then return C.cancel() and false end
  prts.input.rebind(C.capturing, C.scheme, binding)
  prts.settings.save()
  C.capturing = nil
  return true
end

function C.draw()
  C.poll()
  prts.ui.row({ 112, 64, 52, 36, -1 })
  for _, s in ipairs(C.SCHEMES) do
    if prts.ui.button(s.id == C.scheme and ("[" .. s.label .. "]") or s.label, "scheme." .. s.id) then
      C.scheme = s.id
      C.cancel()
    end
  end
  if prts.ui.button("Defaults", "controls.reset") then
    prts.input.reset_bindings(C.scheme)
    prts.settings.save()
    C.cancel()
  end
  prts.ui.row({ -1 })
  if C.capturing then
    prts.ui.label("Press the new key or button for " .. C.capture_label .. " (Esc cancels)")
  else
    prts.ui.label("Choose Change next to an action to rebind it.")
  end
  prts.ui.row({ -1 }, -6)
  prts.ui.panel("controls.list", function()
    prts.ui.row({ 110, -60, -1 })
    for _, a in ipairs(prts.input.actions()) do
      prts.ui.label(a.label)
      prts.ui.label(glyphs.list(a.id, C.scheme))
      if prts.ui.button("Change", "rebind." .. a.id) then C.change(a.id, a.label) end
    end
  end)
end

return C
