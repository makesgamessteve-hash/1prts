-- Spec (rts-core): A barracks trains men-at-arms; No food, no soldiers. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "a barracks trains men-at-arms, and waits for food",
  max_seconds = 120,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local D = require("rts.defs")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local side = S.side(1)
    side.gold, side.timber = 1000, 1000
    local barracks = B.place(1, "crown.barracks", V.WEST.x + 40, V.WEST.z + 40, true)
    B.place(1, "crown.farmstead", V.WEST.x - 40, V.WEST.z + 40, true)
    local u = D.unit("crown.men_at_arms")
    t.expect(B.order_unit(barracks, "crown.men_at_arms"), "the order is taken").is_true()
    t.expect(side.gold, "the gold is spent").equals(1000 - u.cost.gold)
    t.wait_until(function() return #S.units(1, "crown.men_at_arms") == u.squad end, u.train + 5)
    local x, _, z = S.pos(S.units(1, "crown.men_at_arms")[1])
    t.expect(S.dist(x, z, V.WEST.x + 40, V.WEST.z + 40) < 30, "the squad stands at the barracks").is_true()
    -- the rally point: the squad walks out of the door
    t.ticks(150)
    -- no food: fill the folk up to the food and order again
    while S.folk(1) < S.food(1) do S.spawn_unit(1, "crown.serf", V.WEST.x, V.WEST.z + 20) end
    local gold = side.gold
    local ok, why = B.order_unit(barracks, "crown.crossbowmen")
    t.expect(ok, "the order waits").equals(false)
    t.expect(why, "for food").equals("no food")
    t.expect(side.gold, "nothing is paid").equals(gold)
    local last = S.match().dispatches[#S.match().dispatches]
    t.expect(last.text:find("more food") ~= nil, "the quartermaster says so: " .. last.text).is_true()
  end,
}
