-- The front-end menu (start a map, options, credits, quit) and the pause menu while
-- playing (Esc or Start). Menus take the pointer and stop game input while open. MIT License.
local session = require("game.session")
local settings = require("game.settings")
local options = require("ui.options")
local credits = require("ui.credits")
local campaign = require("game.campaign")
local editor = require("ui.editor")

local M = { screen = nil }

-- The game's name (codename 1stPRTS) and its subtitle.
M.TITLE = "Banner & Bastion"
M.SUBTITLE = "War through a commander's eyes"

local GOLD = { 240, 200, 120, 255 }
local PALE = { 225, 214, 190, 255 }

-- Esc, Start and Y on the left XR controller open and close menus; this action works while a
-- menu blocks game input.
prts.input.action{ id = "menu", label = "Menu", keys = { "Escape" }, pad_buttons = { 6 }, xr_buttons = { 5 },
                   always = true }

-- Shows a screen ("main", "pause", "options", "credits", "campaign", "battles") or closes the
-- menu (nil).
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
    -- Menus are drawn on the flat page (none in the headset yet): in VR the menu leaves VR.
    prts.xr.exit()
    if session.current then M.open("pause") end
  elseif M.screen == "pause" then
    M.open(nil)
  elseif M.screen ~= "main" then
    M.open(M.home())
  end
  return M.screen
end

local function centered(w, h, ww, wh) return math.floor((w - ww) / 2), math.floor((h - wh) / 2) end

-- A column of buttons under the title; it scrolls when there are more than fit the screen.
local function list_window(title, w, h, buttons, width)
  local full = 9 + 21 * #buttons
  local ww, wh = math.min(width or 200, w - 16), math.min(full, h - 100)
  local x, y = centered(w, h, ww, wh)
  prts.ui.window(title, x, y + 24, ww, wh, { title = false, scroll = wh < full }, function()
    prts.ui.row({ -1 })
    for _, b in ipairs(buttons) do
      if prts.ui.button(b[1]) then b[2]() end
    end
  end)
end

-- The largest whole-number scale (up to 4) at which the title fits the width, at least 1.
function M.title_scale(w)
  local scale = 4
  while scale > 1 and prts.ui.text_width(M.TITLE) * scale > w - 16 do scale = scale - 1 end
  return scale
end

function M.draw_title(w)
  prts.ui.draw_text(M.TITLE, w / 2, 34, { align = "center", scale = M.title_scale(w), color = GOLD })
  prts.ui.draw_text(M.SUBTITLE, w / 2, 74, { align = "center", color = PALE })
end

-- Starts a map from the menu: with its prologue unless the player turned it off.
function M.start_map(id)
  if not session.start(id, { prologue = settings.game().prologue }) then return false end
  M.open(nil)
  return true
end

-- Starts a campaign act (from the campaign screen or the pause menu).
function M.start_act(id)
  if not campaign.start(id) then return false end
  M.open(nil)
  return true
end

-- The campaign: Continue, then every act (locked ones say so; done ones are ticked).
function M.campaign_buttons()
  local nxt = campaign.next_act()
  local buttons = { { "Continue: " .. nxt.title, function() M.start_act(nxt.id) end } }
  for _, a in ipairs(campaign.ACTS) do
    local label = a.title
    if campaign.done(a.id) then label = label .. "  (done)"
    elseif not campaign.open(a.id) then label = label .. "  (locked)" end
    buttons[#buttons + 1] = { label, function() M.start_act(a.id) end }
  end
  buttons[#buttons + 1] = { "Back", function() M.open(M.home()) end }
  return buttons
end

function M.draw_campaign(w, h)
  M.draw_title(w)
  list_window("The Skyfall Wars", w, h, M.campaign_buttons(), 280)
end

function M.draw_main(w, h)
  M.draw_title(w)
  local first = session.maps[1]
  local buttons = { { "Campaign: The Skyfall Wars", function() M.open("campaign") end },
                    { first.name, function() M.start_map(first.id) end },
                    { "Battles: skirmish and scenarios", function() M.open("battles") end },
                    { "Map editor", function() M.open("editor") end } }
  buttons[#buttons + 1] = { "Options", function() M.open("options") end }
  buttons[#buttons + 1] = { "Credits", function() M.open("credits") end }
  buttons[#buttons + 1] = { "Quit", prts.app.quit }
  list_window("Main menu", w, h, buttons)
end

-- Every other map: the skirmishes and scenarios.
function M.battle_buttons()
  local buttons = {}
  for i, map in ipairs(session.maps) do
    if i > 1 then buttons[#buttons + 1] = { map.name, function() M.start_map(map.id) end } end
  end
  buttons[#buttons + 1] = { "Back", function() M.open(M.home()) end }
  return buttons
end

function M.draw_battles(w, h)
  M.draw_title(w)
  list_window("Battles", w, h, M.battle_buttons(), 300)
end

-- Opens the editor on map file `name` (a new one if it does not exist yet).
function M.edit(name)
  prts.settings.set("editor.file", name)
  prts.settings.save()
  if session.current then return prts.app.restart("editor") end
  if not session.start("editor") then return false end
  M.open(nil)
  return true
end

-- Plays map file `name`.
function M.play(name)
  prts.settings.set("editor.file", name)
  prts.settings.save()
  if session.current then return prts.app.restart("custom") end
  if not session.start("custom") then return false end
  M.open(nil)
  return true
end

-- The map editor's screen: a new map, and every map file to edit or play.
function M.editor_buttons()
  local buttons = { { "New map", function() M.edit(editor.new_name()) end } }
  for _, f in ipairs(editor.files()) do
    buttons[#buttons + 1] = { "Edit: " .. f, function() M.edit(f) end }
    buttons[#buttons + 1] = { "Play: " .. f, function() M.play(f) end }
  end
  buttons[#buttons + 1] = { "Back", function() M.open(M.home()) end }
  return buttons
end

-- The map's settings in the editor: each change rebuilds the ground (the editor starts again).
function M.mapset_buttons()
  local map = editor.map
  local function cycle(list, v)
    for i, x in ipairs(list) do if x == v then return list[i % #list + 1] end end
    return list[1]
  end
  return {
    { "Size: " .. map.size .. " m", function() editor.set("size", cycle({ 512, 1024, 2048 }, map.size)) end },
    { "Hills: " .. map.hills .. " m", function() editor.set("hills", cycle({ 0, 6, 12, 24, 40 }, map.hills)) end },
    { "Time: " .. map.time, function() editor.set("time", cycle(require("game.mapfile").TIMES, map.time)) end },
    { "New ground (seed " .. map.seed .. ")", function() editor.set("seed", map.seed + 1) end },
    { "Back", function() M.open("pause") end },
  }
end

function M.pause_buttons()
  local buttons = { { "Resume", function() M.open(nil) end } }
  if session.current == "editor" then
    buttons[#buttons + 1] = { "Save map", function() editor.save() M.open(nil) end }
    buttons[#buttons + 1] = { "Play this map", function() editor.play() end }
    buttons[#buttons + 1] = { "Map settings", function() M.open("mapset") end }
  elseif session.current == "custom" then
    buttons[#buttons + 1] = { "Back to the editor", function() M.edit(prts.settings.get("editor.file") or "map_1.map") end }
  end
  local offer = campaign.offer() -- after an act: the next one, or this one again
  if offer then buttons[#buttons + 1] = { offer[1], function() M.start_act(offer[2]) end } end
  buttons[#buttons + 1] = { "Campaign", function() M.open("campaign") end }
  buttons[#buttons + 1] = { "Options", function() M.open("options") end }
  buttons[#buttons + 1] = { "Credits", function() M.open("credits") end }
  buttons[#buttons + 1] = { "Quit", prts.app.quit }
  return buttons
end

function M.draw_pause(w, h)
  M.draw_title(w)
  list_window("Game menu", w, h, M.pause_buttons())
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
  elseif M.screen == "campaign" then
    M.draw_campaign(w, h)
  elseif M.screen == "battles" then
    M.draw_battles(w, h)
  elseif M.screen == "editor" then
    M.draw_title(w)
    list_window("Map editor", w, h, M.editor_buttons(), 260)
  elseif M.screen == "mapset" then
    M.draw_title(w)
    list_window("Map settings", w, h, M.mapset_buttons(), 220)
  end
end

prts.events.on("ui", "ui.menu", M.frame)
return M
