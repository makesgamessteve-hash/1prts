-- A gold mine worked from inside a rock hill: an adit at ground level, a lower gallery, a lift
-- cage in a shaft, and an upper gallery with a gold vein and a lookout over the land. The
-- galleries are a deck plan; the rock between and above them is solid. MIT License.
local Deck = require("lib.deckplan")
local Lift = require("props.lift")

local M = {}
M.CELL = 1.2
M.UPPER = 8 -- the upper gallery's floor above the lower one

M.PLAN = {
  { y = 0, height = 3, rows = {
    "##########",
    "#aaaaaaHH#",
    "#a####aHH#",
    "#a####aHH#",
    "#a########",
    "#D########",
  } },
  { y = M.UPPER, height = 3, rows = {
    "##########",
    "#uuuuuuHH#",
    "#u####uHH#",
    "#u####uHH#",
    "#u########",
    "#W########",
  } },
}

-- cfg = { name, pos = { x, z } (the adit's foot, on the ground) }. The mine runs north of it.
function M.make(cfg)
  local c = M.CELL
  local ax, az = cfg.pos[1], cfg.pos[2]
  local ground = prts.scene.height(ax, az)
  local floor = ground + 0.2
  -- the plan's north-west corner: the adit is column 2 of the last row
  local ox, oz = ax - 1.5 * c, az - 6 * c
  local e = prts.entity.spawn{ name = cfg.name, pos = { ox, floor, oz } }
  local plan = Deck.compile{ entity = e, cell = c, origin = { 0, 0, 0 }, material = "stone_floor", wall = "rock",
                             levels = M.PLAN, prefix = cfg.name, rooms = { a = "lower gallery", u = "upper gallery" } }
  local w, d = 10 * c, 6 * c
  local function rock(x0, y0, z0, x1, y1, z1)
    prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = "rock", entity = e }
  end
  -- solid rock between the galleries and over the top, but not in the shaft (columns 8-9, rows 2-4)
  local sx0, sx1, sz0, sz1 = 7 * c, 9 * c, 1 * c, 4 * c
  local y0, y1 = 3, M.UPPER - 0.2
  rock(0, y0, 0, sx0, y1, d)
  rock(sx1, y0, 0, w, y1, d)
  rock(sx0, y0, 0, sx1, y1, sz0)
  rock(sx0, y0, sz1, sx1, y1, d)
  rock(-1, M.UPPER + 3, -1, w + 1, M.UPPER + 4.5, d + 1) -- the hill's crown
  rock(-1, -1, -1, 0, M.UPPER + 3, d + 1)                 -- and its flanks, beyond the plan
  rock(w, -1, -1, w + 1, M.UPPER + 3, d + 1)
  rock(-1, -1, -1, w + 1, M.UPPER + 3, 0)
  -- the gold vein in the upper gallery's north wall, and lanterns
  prts.scene.box{ min = { 1.4, M.UPPER + 0.6, c - 0.02 }, max = { 6.0, M.UPPER + 1.8, c + 0.02 }, material = "gold_vein",
                  entity = e, solid = false }
  prts.scene.light{ pos = { 1.8, 2.4, 6 }, color = { 1.0, 0.75, 0.45 }, radius = 8, entity = e }
  prts.scene.light{ pos = { 5, 2.4, 1.8 }, color = { 1.0, 0.75, 0.45 }, radius = 8, entity = e }
  prts.scene.light{ pos = { 4, M.UPPER + 2.4, 1.8 }, color = { 1.0, 0.8, 0.5 }, radius = 9, entity = e }
  -- the lift cage in the shaft, with its gates on the gallery side
  local lift = Lift.make{ name = cfg.name .. ".cage", pos = { ox + 8 * c, floor, oz + 2.5 * c },
                          floors = { floor, floor + M.UPPER }, size = { 2.2, 3.4 }, gate = ox + 7 * c - 0.06,
                          material = "planks", gate_material = "iron", lever_material = "oak" }
  local data = prts.entity.data(e)
  data.kind, data.plan, data.lift = "mine", plan, prts.entity.name(lift)
  -- the rooms as zones of the world, for tests and stories
  for name, r in prts.sorted_pairs(plan.rooms) do
    prts.scene.zone(cfg.name .. "." .. name, { ox + r.min[1], floor + r.min[2] - 0.2, oz + r.min[3] },
                    { ox + r.max[1], floor + r.max[2], oz + r.max[3] })
  end
  return e
end

-- The lift cage's platform entity of a mine.
function M.cage(mine) return prts.entity.find(prts.entity.data(mine).lift) end

return M
