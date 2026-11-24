-- The map editor's workshop (M10): the map file named by the setting editor.file, shown with a
-- stand-in for every object so the commander can walk it and change it with the surveyor's rod
-- (ui/editor.lua). Nothing fights here: there is no match. MIT License.
local Mf = require("game.mapfile")
local Mb = require("game.mapbuild")

local M = {}
M.ID = "editor"

function M.build()
  local name = prts.settings.get("editor.file") or "map_1.map"
  local map, why = Mf.load(name)
  if not map then map = Mf.new(name:gsub("%.map$", "")) end
  M.map, M.name, M.why = map, name, why
  Mb.ground(map)
  Mb.start(map)
  require("ui.editor").open(map, name)
  return true
end

prts.events.on("session_started", "maps.editor", function(id)
  if id ~= M.ID then return false end
  require("ui.banner").show("Map editor: " .. M.map.name, "The surveyor's rod is in your hand", 0.7)
  return true
end)
return M
