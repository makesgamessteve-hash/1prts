-- A map made with the editor, played (M10): the map file named by the setting editor.file
-- (game/mapfile.lua), built by game/mapbuild.lua. With two or more holdfasts it is a match (side
-- 1 the player's, the others the AI's), with the Custodians at its ruins. MIT License.
local Mf = require("game.mapfile")
local Mb = require("game.mapbuild")

local M = {}
M.ID = "custom"
M.music = "music/proving_ground.ogg"

-- The map file being played (a broken or missing one plays as an empty map, and says why).
function M.file()
  local name = prts.settings.get("editor.file") or "map_1.map"
  local map, why = Mf.load(name)
  if not map then
    prts.log.warn("map " .. name .. ": " .. tostring(why))
    map = Mf.new(name)
  end
  return map, name, why
end

function M.build()
  local map = M.file()
  M.map = map
  Mb.ground(map)
  Mb.play(map)
  Mb.start(map)
  return true
end

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  local h = M.map.size / 2 - 24
  prts.nav.build{ min = { -h, -h }, max = { h, h }, cell = 2 }
  require("rts.command").AREA = { -h, -h, h, h }
  if opts and opts.no_match then return true end
  Mb.match(M.map)
  require("ui.banner").show(M.map.name, "A map from the editor", 0.7)
  return true
end

prts.events.on("session_started", "maps.custom", M.on_started)
return M
