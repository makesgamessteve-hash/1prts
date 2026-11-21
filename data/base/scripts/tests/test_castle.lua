-- Lua unit tests: castle building blocks. MIT License.
local Castle = require("lib.castle")
local Portcullis = require("props.portcullis")

T.case("battlements space their merlons evenly", function()
  T.eq(Castle.battlements("stone", "x", 0, 9, 0, 0.6, 10), 5)
  T.eq(Castle.battlements("stone", "z", 0, 1, 0, 0.6, 10), 1, "always at least one")
end)

T.case("tower sides face the given directions", function()
  T.eq(Castle.side_towards(16, 1, 0), 0)
  T.eq(Castle.side_towards(16, 0, 1), 4)
  T.eq(Castle.side_towards(16, -1, 0), 8)
  T.eq(Castle.side_towards(16, 0, -1), 12)
  T.eq(Castle.side_towards(16, 1, 1), 2)
end)

T.case("round towers: bands around doors, roofs and stairs", function()
  local top, stair, bands = Castle.round_tower{ x = 500, z = 500, radius = 5, height = 12, material = "stone",
                                                doors = { { side = 0, y = 0 }, { side = 4, y = 6, height = 2 } },
                                                floors = { 6 }, roof = "cone", light = false }
  T.eq(top, 12)
  T.eq(stair, nil)
  T.eq(bands, 5, "below ground, the ground door, between, the upper door and above it")
  top, stair = Castle.round_tower{ x = 520, z = 500, radius = 6, height = 9, material = "stone",
                                   stair = { radius = 2, size = 1.2, rise = 0.3 }, light = { 1, 1, 1 } }
  T.eq(top, 9)
  T.near(stair, 9, 1e-9)
end)

T.case("walls, stairs, halls and gatehouses", function()
  T.eq(Castle.curtain("stone", "x", 0, 20, 500, 503, 8), 8)
  T.eq(Castle.wall_stair("stone", { axis = "z", sign = 1, start = 600, w0 = 0, w1 = 1, base = 0, top = 5 }), 20)
  Castle.hall{ x0 = 600, z0 = 600, x1 = 610, z1 = 604, height = 4, material = "plaster" } -- no door
  local grille = Castle.gatehouse{ name = "test.gate", x = 700, z0 = 700, z1 = 708, half = 8, height = 9, outward = -1,
                                   passage = { half = 2, height = 4 }, tower = { radius = 4, height = 12 },
                                   material = "stone" }
  T.is_true(prts.entity.data(grille).open, "starts open")
  T.eq(Portcullis.set(grille, true), false, "already open")
  T.eq(Portcullis.toggle(grille), true)
  T.eq(prts.entity.data(grille).open, false)
  T.eq(Portcullis.on_use(grille), false, "the grille itself is not the winch")
end)
