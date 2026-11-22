-- The minimal HUD (First-Person Contract): the interaction prompt when the commander looks at
-- something usable, with the key, button or touch glyph of the active scheme. Subtitles are
-- drawn by ui/subtitles.lua. MIT License.
local glyphs = require("ui.glyphs")

local H = { scheme = prts.input.scheme(), visible = true }

-- What using an object does, by the kind its script stores in its entity data.
H.PROMPTS = {
  door = function(data) return data.open and "Close door" or "Open door" end,
  lift_lever = function(data)
    local lift = prts.entity.data(data.lift)
    return lift.current == #lift.floors and "Pull the lever: down" or "Pull the lever: up"
  end,
  lift_call = "Call the lift",
  horse = "Mount the horse",
  post = function(data) return data.taken and ("Leave the " .. data.label) or ("Take the " .. data.label) end,
  npc = function(data) return "Talk to " .. data.person end,
  portcullis_winch = function(data)
    return prts.entity.data(data.grille).open and "Lower the portcullis" or "Raise the portcullis"
  end,
}

function H.prompt_text(entity)
  local data = prts.entity.data(entity)
  local p = H.PROMPTS[data.kind]
  if type(p) == "function" then p = p(data) end
  return p or data.prompt or "Use"
end

-- The whole prompt line ("[E] Open door"), or nil when nothing usable is in view.
function H.prompt()
  local target = prts.commander.target()
  if not target then return nil end
  return glyphs.prompt("use", H.scheme) .. " " .. H.prompt_text(target)
end

-- Under the water (M8): a breath bar while breath is not full, with "Breath" written on it.
function H.draw_breath(w, h)
  local Wa = require("rts.water")
  local left, full = Wa.breath()
  if left >= full then return false end
  local bw = 120
  local x, y = math.floor(w / 2 - bw / 2), math.floor(h * 0.12)
  prts.ui.draw_rect(x - 2, y - 2, bw + 4, 10, { 0, 0, 0, 150 })
  local col = left > full * 0.25 and { 120, 200, 230, 255 } or { 230, 90, 70, 255 }
  prts.ui.draw_rect(x, y, math.max(1, math.floor(bw * left / full)), 6, col)
  prts.ui.draw_text("Breath", w / 2, y + 9, { align = "center", color = { 220, 240, 250, 255 } })
  return true
end

function H.draw(w, h)
  if not H.visible or prts.ui.modal() then return false end
  if prts.dev and prts.dev.observer() then return false end -- the commander's view only
  H.draw_breath(w, h)
  local text = H.prompt()
  if not text then return false end
  local tw = prts.ui.text_width(text)
  local y = math.floor(h * 0.62)
  prts.ui.draw_rect(math.floor(w / 2 - tw / 2) - 4, y - 3, tw + 8, 15, { 0, 0, 0, 150 })
  prts.ui.draw_text(text, w / 2, y, { align = "center", color = { 250, 236, 200, 255 } })
  return true
end

function H.on_scheme(scheme) H.scheme = scheme end

prts.events.on("input_scheme", "ui.hud", H.on_scheme)
prts.events.on("ui", "ui.hud", function(surface, w, h)
  if surface == "screen" then H.draw(w, h) end
end)

return H
