-- Presence: the commander's banner close by makes their people do better (concept 02 §4).
-- Within RANGE metres of the commander: companies hold longer and strike harder, builders build
-- a quarter faster, firefighters work a fifth faster. Only the side the commander leads has it.
-- MIT License.
local Pr = {}
Pr.RANGE = 50
Pr.BUILD, Pr.FIRE, Pr.STRIKE = 1.25, 1.2, 1.15
Pr.BREAK = 3 -- with presence a company breaks at a third of its strength (else at half)

function Pr.near(side_id, x, z)
  local Cmd = require("rts.command")
  if Cmd.side() ~= side_id then return false end
  local cx, cz = Cmd.here()
  if not cx then return false end
  local fell = require("rts.succession").fallen()
  return not fell and (cx - x) ^ 2 + (cz - z) ^ 2 <= Pr.RANGE * Pr.RANGE
end

return Pr
