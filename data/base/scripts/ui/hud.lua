-- The minimal HUD (First-Person Contract): the interaction prompt when the commander looks at
-- something usable, with the key, button or touch glyph of the active scheme. Subtitles are
-- drawn by ui/subtitles.lua. MIT License.
local glyphs = require("ui.glyphs")

local H = { scheme = prts.input.scheme(), visible = true }

-- What using an object does, by the kind its script stores in its entity data.
H.PROMPTS = {
  door = function(data) return data.open and "Close door" or "Open door" end,
  lift_lever = "Pull the lever",
  lift_call = "Call the lift",
  npc = function(data) return "Talk to " .. data.person end,
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

function H.draw(w, h)
  if not H.visible or prts.ui.modal() then return false end
  if prts.dev and prts.dev.observer() then return false end -- the commander's view only
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
