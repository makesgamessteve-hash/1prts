-- Banner & Bastion (codename 1stPRTS) base game boot script. MIT License.
-- The engine runs this file after mounting the data folder. Everything game-specific
-- (controls, menus, the map, what the commander starts with) is decided here, not in the engine.
local input = require("input.defaults")
input.apply()
require("game.settings").apply()
require("ui.touch").apply()
require("ui.hud")
require("game.feel").apply_settings() -- footsteps, landings and the view's own motion
require("ui.subtitles")
require("story.harrowmark") -- the prologue, when a map is started from the menu
local menu = require("ui.menu")
local session = require("game.session")

-- Windowed runs show the front-end menu; test, batch and --start runs begin a map at once.
if prts.boot.auto_start then
  session.start(prts.boot.map or "proving_ground")
else
  menu.open("main")
  prts.audio.music{ track = "music/menu.ogg", fade = 1 }
end
