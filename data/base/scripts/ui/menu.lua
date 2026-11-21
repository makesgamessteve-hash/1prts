-- The front-end menu (start a map, options, credits, quit) and the pause menu while
-- playing (Esc or Start). Menus take the pointer and stop game input while open. MIT License.
local session = require("game.session")
local options = require("ui.options")
local credits = require("ui.credits")

local M = { screen = nil }

local GOLD = { 240, 200, 120, 255 }
local PALE = { 225, 214, 190, 255 }

-- Esc and Start open and close menus; this action works while a menu blocks game input.
prts.input.action{ id = "menu", label = "Menu", keys = { "Escape" }, pad_buttons = { 6 }, always = true }

-- Shows a screen ("main", "pause", "options", "credits") or closes the menu (nil).
function M.open(screen)
  M.screen = screen
  prts.ui.set_modal(screen ~= nil)
  if screen == "credits" then credits.refresh() end
end

-- Where Back leads: the pause menu while playing, else the front-end.
function M.home() return session.current and "pause" or "main" end

function M.on_menu_key()
  if options.cancel_capture() then return M.screen end
  if M.screen == nil then
    if session.current then M.open("pause") end
  elseif M.screen == "pause" then
    M.open(nil)
  elseif M.screen ~= "main" then
    M.open(M.home())
  end
  return M.screen
end

local function centered(w, h, ww, wh) return math.floor((w - ww) / 2), math.floor((h - wh) / 2) end

local function list_window(title, w, h, buttons)
  local ww, wh = 200, 9 + 21 * #buttons
  local x, y = centered(w, h, ww, wh)
  prts.ui.window(title, x, y + 24, ww, wh, { title = false, scroll = false }, function()
    prts.ui.row({ -1 })
    for _, b in ipairs(buttons) do
      if prts.ui.button(b[1]) then b[2]() end
    end
  end)
end

function M.draw_title(w)
  prts.ui.draw_text("1stPRTS", w / 2, 34, { align = "center", scale = 4, color = GOLD })
  prts.ui.draw_text("A first-person fantasy RTS", w / 2, 74, { align = "center", color = PALE })
end

function M.draw_main(w, h)
  M.draw_title(w)
  local buttons = {}
  for _, map in ipairs(session.maps) do
    buttons[#buttons + 1] = { map.name, function() if session.start(map.id) then M.open(nil) end end }
  end
  buttons[#buttons + 1] = { "Options", function() M.open("options") end }
  buttons[#buttons + 1] = { "Credits", function() M.open("credits") end }
  buttons[#buttons + 1] = { "Quit", prts.app.quit }
  list_window("Main menu", w, h, buttons)
end

function M.draw_pause(w, h)
  M.draw_title(w)
  list_window("Game menu", w, h, {
    { "Resume", function() M.open(nil) end },
    { "Options", function() M.open("options") end },
    { "Credits", function() M.open("credits") end },
    { "Quit", prts.app.quit },
  })
end

function M.frame(surface, w, h)
  if surface ~= "screen" then return end
  if prts.input.went_down("menu") then M.on_menu_key() end
  local back = function() M.open(M.home()) end
  if M.screen == "main" then
    M.draw_main(w, h)
  elseif M.screen == "pause" then
    M.draw_pause(w, h)
  elseif M.screen == "options" then
    options.draw(w, h, back)
  elseif M.screen == "credits" then
    credits.draw(w, h, back)
  end
end

prts.events.on("ui", "ui.menu", M.frame)
return M
