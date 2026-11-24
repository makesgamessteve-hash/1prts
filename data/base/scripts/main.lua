-- Banner & Bastion (codename 1stPRTS) base game boot script. MIT License.
-- The engine runs this file after mounting the data folder. Everything game-specific
-- (controls, menus, the map, what the commander starts with) is decided here, not in the engine.
require("lib.i18n").install() -- M10: the UI's text is translated as it is drawn
local input = require("input.defaults")
input.apply()
require("game.settings").apply()
require("ui.touch").apply()
require("ui.hud")
require("game.feel").apply_settings() -- footsteps, landings and the view's own motion
require("ui.subtitles")
require("story.harrowmark") -- the prologue, when a map is started from the menu
require("ui.editor") -- the map editor's rod (M10)
local menu = require("ui.menu")
local session = require("game.session")

-- Each enabled mod's entry point, scripts/mods/<id>/init.lua, runs now in load order (after the
-- base game's modules, before the first map): mods add maps, units, languages and rules there.
-- A mod whose entry point fails is reported and skipped; the game carries on.
for _, m in ipairs(prts.vfs.mounts()) do
  local path = "scripts/mods/" .. m.id .. "/init.lua"
  if m.id ~= "base" and prts.vfs.read_from(m.id, path) then
    local ok, err = pcall(require, "mods." .. m.id .. ".init")
    if ok then prts.log.info("mod " .. m.id .. " started") else prts.log.error("mod " .. m.id .. ": " .. tostring(err)) end
  end
end

-- Windowed runs show the front-end menu; test, batch and --start runs begin a map at once.
if prts.boot.auto_start then
  -- a campaign act handed over by prts.app.restart (game/campaign.lua) begins on its map
  local map = prts.boot.map or "proving_ground"
  local act = require("game.campaign").pending(map)
  session.start(map, act and { campaign = act, prologue = act == "prologue" } or nil)
else
  menu.open("main")
  prts.audio.music{ track = "music/menu.ogg", fade = 1 }
end
