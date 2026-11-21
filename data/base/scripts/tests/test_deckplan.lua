-- Lua unit tests: the deck-plan compiler (lib/deckplan.lua), on a patch of open ground beside
-- Harrowmark. MIT License.
local P = require("lib.deckplan")

T.case("a small two-level plan: floors, walls, a doorway, a stair and a room", function()
  local e = prts.entity.spawn{ name = "test.plan", pos = { 120, prts.scene.height(120, 120), 120 } }
  local plan = P.compile{ entity = e, cell = 1, prefix = "test", material = "planks", wall = "oak", roof = "planks",
    rooms = { r = "store" },
    levels = {
      { y = 0, height = 3, rows = {
        "#####",
        "#s.r#",
        "#s.r#",
        "#s.r#",
        "#.W.#",
        "##D##",
      } },
      { y = 3, height = 3, rows = {
        "#####",
        "#..S#",
        "#..S#",
        "#..S#",
        "#...#",
        "#####",
      } },
    } }
  T.is_true(plan.boxes > 10, "boxes were made (" .. plan.boxes .. ")")
  T.eq(#plan.stairs, 2, "one stair per level")
  T.eq(plan.stairs[1].up, 1)
  T.near(plan.stairs[1].y1 - plan.stairs[1].y0, 3, 1e-6, "climbs one level")
  T.eq(plan.stairs[2].up, -1, "the upper stair rises towards -z")
  local room = plan.rooms.store
  T.is_true(room ~= nil, "the room is named")
  T.eq(room.cells, 3)
  T.near(room.min[1], 3, 1e-6)
  T.near(room.max[1], 4, 1e-6)
  T.near(room.min[3], 1, 1e-6)
  T.near(room.max[3], 4, 1e-6)
  T.eq(P.room_at(plan, 3.5, 0.5, 2.5), "store")
  T.eq(P.room_at(plan, 2.5, 0.5, 2.5), nil, "the hall is no room")
  local cx, cy, cz = P.room_centre(plan, "store")
  T.near(cx, 3.5, 1e-6)
  T.near(cy, 0, 1e-6)
  T.near(cz, 2.5, 1e-6)
  T.eq(P.room_centre(plan, "nowhere"), nil)
  -- frames: there and back again
  local wx, wy, wz = P.to_world(e, 1, 2, 3)
  local lx, ly, lz = P.to_frame(e, wx, wy, wz)
  T.near(lx, 1, 1e-6)
  T.near(ly, 2, 1e-6)
  T.near(lz, 3, 1e-6)
  prts.entity.despawn(e)
end)
