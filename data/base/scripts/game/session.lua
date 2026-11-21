-- A play session: building a map, placing the commander and finishing the load. The
-- front-end menu starts one; test and batch runs start one at boot. MIT License.
local S = { current = nil }

-- Maps offered by the front-end menu (mods can add to this list).
S.maps = {
  { id = "proving_ground", name = "Proving Ground" },
}

-- Starts map `id` (scripts/maps/<id>.lua). Only one map per run in M1: returns false if a
-- map is already loaded.
function S.start(id)
  if S.current then return false end
  local map = require("maps." .. id)
  map.build()
  prts.commander.spawn{ marker = "start" }
  prts.scene.finish()
  S.current = id
  prts.log.info(id .. " ready (scripting API " .. prts.api_version .. ")")
  prts.events.emit("session_started", id)
  return true
end

return S
