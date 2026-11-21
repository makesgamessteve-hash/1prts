-- 1stPRTS base game boot script. MIT License.
-- The engine runs this file after mounting the data folder. Everything game-specific
-- (controls, the map, what the commander starts with) is decided here, not in the engine.
local input = require("input.defaults")
input.apply()

prts.render.settings{ preset = "retro", dither = true, snap = true, fog = true, crosshair = true, fov = 70 }

local map = require("maps.proving_ground")
map.build()

prts.commander.spawn{ marker = "start" }
prts.log.info("Proving Ground ready (scripting API " .. prts.api_version .. ")")
