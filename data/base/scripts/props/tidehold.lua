-- The Tidehold (M8): the Brine League's raft-city, a headquarters that sails. Rafts lashed into a
-- 30 x 40 m deck of planks with huts, a market row, masts with sails and, amidships, the council
-- deck under an awning with a great war table of inlaid shell. Whoever stands on it rides along;
-- broadsides hit it like any building, and it sinks when it falls. It faces -z in its own frame.
-- MIT License.
local S = require("rts.state")

local Th = {}
Th.TABLE = { 0, 0, -2 }

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

function Th.build(e)
  box(e, "oak", -15, -1.2, -20, 15, -0.2, 20)                -- the rafts under the deck
  box(e, "planks", -15, -0.2, -20, 15, 0, 20)                 -- the deck
  for _, s in ipairs({ -1, 1 }) do                            -- low rails along the sides
    box(e, "oak", s > 0 and 14.8 or -15, 0, -20, s > 0 and 15 or -14.8, 0.9, -3)
    box(e, "oak", s > 0 and 14.8 or -15, 0, 3, s > 0 and 15 or -14.8, 0.9, 20)
  end
  for i = 0, 3 do                                             -- huts along the stern
    local x = -12 + i * 8
    box(e, "planks", x - 2.5, 0, 12, x + 2.5, 3, 18)
    prts.scene.wedge{ min = { x - 2.8, 3, 11.7 }, max = { x + 2.8, 4.4, 18.3 }, axis = "x", sign = 1, material = "slate",
                      entity = e }
  end
  for i = 0, 2 do                                             -- the market row's stalls
    local x = -8 + i * 8
    box(e, "oak", x - 2, 0, -16, x + 2, 1, -14)
    box(e, "sail_blue", x - 2.2, 2.4, -16.2, x + 2.2, 2.6, -13.8, false)
  end
  for _, z in ipairs({ -10, 8 }) do                           -- masts and sails
    box(e, "oak", -0.25, 0, z - 0.25, 0.25, 16, z + 0.25, false)
    box(e, "sail_blue", -6, 5, z - 0.1, 6, 14, z + 0.1, false)
  end
  box(e, "sail_blue", -5, 3.4, -6, 5, 3.6, 2, false)          -- the council deck's awning
  for _, c in ipairs({ { -5, -6 }, { 5, -6 }, { -5, 2 }, { 5, 2 } }) do
    box(e, "oak", c[1] - 0.15, 0, c[2] - 0.15, c[1] + 0.15, 3.4, c[2] + 0.15, false)
  end
  box(e, "banner_blue", -0.05, 16, -10, 0.05, 19, -8, false)
  prts.scene.light{ pos = { 0, 3, -2 }, color = { 1.0, 0.85, 0.6 }, radius = 12, entity = e }
  return e
end

-- The Tidehold of a side floated out at (x, z), heading north (-z). Returns its entity.
function Th.spawn(side_id, x, z)
  local St = require("rts.structures")
  local e = St.place(side_id, "brine.tidehold", x, z, true)
  prts.entity.place(e, { x, (require("rts.water").level() or 0) + require("rts.defs").structure("brine.tidehold").freeboard, z }, 0)
  Th.build(e)
  S.dispatch(side_id, "Harbourmaster", "The Tidehold casts off. The city sails!")
  prts.events.emit("rts_tidehold", e, side_id)
  return e
end

-- Sends the Tidehold to (x, z) (it keeps off the shallows).
function Th.order(e, x, z)
  S.data(e).dest = { x, z }
  return true
end

-- The council deck's war table (for a League commander).
function Th.furnish(e)
  return require("rts.tools").furnish(e, Th.TABLE)
end

-- Where a commander stands at the council table: x, y, z.
function Th.council_point(e)
  return require("lib.deckplan").to_world(e, 0, 0.1, Th.TABLE[3] + 2.6)
end

return Th
