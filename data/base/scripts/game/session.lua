-- A play session: building a map, placing the commander and finishing the load. The
-- front-end menu starts one; test and batch runs start one at boot. MIT License.
local S = { current = nil }

-- Maps offered by the front-end menu (mods can add to this list): the map id
-- (scripts/maps/<id>.lua) and the name the menu shows.
S.maps = {
  { id = "proving_ground", name = "Harrowmark" },
  { id = "ashford_vale", name = "Skirmish: Ashford Vale" },
  { id = "greyfen_march", name = "Ride Out: Greyfen March" },
}

-- Starts map `id` (scripts/maps/<id>.lua). Only one map per run in M1: returns false if a
-- map is already loaded. `opts` is passed on with the "session_started" event:
-- `{ prologue = true }` asks the map's story for its prologue (the front-end menu does).
function S.start(id, opts)
  if S.current then return false end
  local map = require("maps." .. id)
  map.build()
  prts.commander.spawn{ marker = "start" }
  prts.scene.finish()
  if map.music then prts.audio.music{ track = map.music, fade = 2 } end
  S.current = id
  prts.log.info(id .. " ready (scripting API " .. prts.api_version .. ")")
  prts.events.emit("session_started", id, opts or {})
  return true
end

return S
