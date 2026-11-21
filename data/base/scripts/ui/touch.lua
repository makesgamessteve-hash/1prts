-- The touch overlay: a floating movement stick on the left half, look-by-drag and tap-to-use
-- on the right half, and context buttons that only show when they can be used. The engine
-- turns fingers into action values; this file sets the layout and draws it. MIT License.
local T = {}

-- Positions are fractions of the screen; radii are fractions of its height. Button i drives
-- touch button i (see input/defaults.lua).
T.LAYOUT = {
  split = 0.5, stick_radius = 0.12, look_speed = 1000, tap_distance = 0.03, tap_seconds = 0.3,
  buttons = {
    { id = "use", label = "Use", x = 0.86, y = 0.50, radius = 0.08 },
    { id = "jump", label = "Jump", x = 0.93, y = 0.74, radius = 0.07 },
    { id = "crouch", label = "Crouch", x = 0.81, y = 0.84, radius = 0.07 },
    { id = "run", label = "Run", x = 0.70, y = 0.89, radius = 0.06 },
    -- the commander's tools, shown during a skirmish
    { id = "field_map", label = "Map", x = 0.94, y = 0.12, radius = 0.055 },
    { id = "stone", label = "Stone", x = 0.94, y = 0.29, radius = 0.055 },
    { id = "mark", label = "Mark", x = 0.82, y = 0.12, radius = 0.055 },
    { id = "horn", label = "Horn", x = 0.82, y = 0.29, radius = 0.055 },
    { id = "next_company", label = "Next", x = 0.70, y = 0.12, radius = 0.05 },
    { id = "mount", label = "Ride", x = 0.70, y = 0.29, radius = 0.05 },
  },
}

function T.apply() return prts.input.touch_layout(T.LAYOUT) end

-- Whether the commander leads a side in a running skirmish (the tools' buttons show then);
-- rts/tools.lua sets T.tools_active when it is loaded; props/horse.lua sets T.can_ride.
function T.in_match()
  return T.tools_active ~= nil and T.tools_active() == true
end

-- Which context buttons can be used right now.
function T.usable()
  local playing = prts.commander.entity() ~= nil and not prts.ui.modal()
  local tools = playing and T.in_match()
  return {
    use = playing and prts.commander.target() ~= nil,
    jump = playing, crouch = playing, run = playing,
    field_map = tools, stone = tools, mark = tools, horn = tools, next_company = tools,
    mount = playing and T.can_ride ~= nil and T.can_ride() == true,
  }
end

function T.update()
  local usable = T.usable()
  for _, b in ipairs(T.LAYOUT.buttons) do prts.input.touch_visible(b.id, usable[b.id]) end
end

local RING = { 255, 240, 210, 150 }
local KNOB = { 255, 240, 210, 110 }

-- Draws the overlay when touch is the active scheme; returns whether it drew.
function T.draw(w, h)
  if prts.input.scheme() ~= "touch" or prts.ui.modal() then return false end
  local state = prts.input.touch_state()
  if state.stick then
    local r = T.LAYOUT.stick_radius * h
    local ox, oy = state.stick[1] * w, state.stick[2] * h
    local kx, ky = state.stick[3] * w, state.stick[4] * h
    local dx, dy = kx - ox, ky - oy
    local len = math.sqrt(dx * dx + dy * dy)
    if len > r then kx, ky = ox + dx * r / len, oy + dy * r / len end
    prts.ui.draw_icon("ring", ox - r, oy - r, 2 * r, 2 * r, RING)
    prts.ui.draw_icon("disc", kx - r * 0.4, ky - r * 0.4, r * 0.8, r * 0.8, KNOB)
  end
  for i, b in ipairs(state.buttons) do
    if b.visible then
      local r, x, y = b.radius * h, b.x * w, b.y * h
      prts.ui.draw_icon(b.held and "disc" or "ring", x - r, y - r, 2 * r, 2 * r, RING)
      prts.ui.draw_text(T.LAYOUT.buttons[i].label, x, y - 4, { align = "center" })
    end
  end
  return true
end

-- Only the touch scheme needs the overlay (and the raycast behind the use button).
prts.events.on("ui", "ui.touch", function(surface, w, h)
  if surface == "screen" and prts.input.scheme() == "touch" then
    T.update()
    T.draw(w, h)
  end
end)

return T
