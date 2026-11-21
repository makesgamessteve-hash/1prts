-- Spec (rts-core): A serf brings gold home. A serf sent to the gold mine near the holdfast is back
-- within a minute with a load: the side's gold grows by the load and the mine shrinks by the same.
-- MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "a serf brings gold home",
  max_seconds = 120,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local E = require("rts.economy")
    local D = require("rts.defs")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local side = S.side(1)
    local serf = S.spawn_unit(1, "crown.serf", V.WEST.x, V.WEST.z + 18)
    t.expect(E.gather(serf, "gold"), "the serf finds a mine").is_true()
    local mine = S.data(serf).task.src
    local gold0, in_mine0 = side.gold, S.data(mine).gold
    t.wait_until(function() return side.gold > gold0 end, 60)
    local load = D.unit("crown.serf").carry
    t.expect(side.gold - gold0, "the side's gold grew by one load").equals(load)
    t.expect(in_mine0 - S.data(mine).gold, "the mine shrank by the same").equals(load)
    -- and the serf goes back for more by itself
    t.ticks(30)
    t.expect(S.data(serf).task.phase, "back to the mine").equals("go")
    -- timber: a tree is felled when its timber runs out, and its ground is free again
    local tree = E.nearest_source("timber", V.WEST.x, V.WEST.z)
    local tx, _, tz = S.pos(tree)
    S.data(tree).timber = load
    local woodcutter = S.spawn_unit(1, "crown.serf", tx + 3, tz)
    E.gather(woodcutter, "timber")
    t.expect(S.data(woodcutter).task.src, "the woodcutter takes the nearest tree").equals(tree)
    local timber0 = side.timber
    t.wait_until(function() return side.timber > timber0 end, 90)
    t.expect(S.alive(tree), "the tree is felled").equals(false)
    t.expect(prts.nav.walkable(tx, tz), "its ground is walkable").is_true()
  end,
}
