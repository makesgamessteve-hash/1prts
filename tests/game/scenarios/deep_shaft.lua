-- Spec (tunnels): Down the deep shaft. Spec (under-the-hill): Caldrith's chain. The commander
-- rides the Crown's deep shaft's lift cage down into the caverns and walks to the chain's
-- cavern. Every faction can sink a shaft at tier 4. MIT License.
-- ctest-args: --start underhill
return {
  name = "down the deep shaft",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local B = require("rts.structures")
    local d = Tu.data()
    local foot = d.nodes.shaft
    local c = t.commander()
    t.expect(foot.lift, "the shaft has its cage").equals("lift.shaft")
    local plat = prts.entity.find("lift.shaft.platform")
    local px, py, pz = t.pos(plat)
    t.place(px, py + 0.1, pz)
    t.ticks(10)
    t.face("lift.shaft.lever")
    t.ticks(2)
    t.expect(t.target(), "the cage's lever").equals("lift.shaft.lever")
    t.input.press("use")
    t.wait_until(function() local _, y = t.pos(c) return y < foot.y + 0.6 end, 60)
    t.ticks(30)
    local _, y = t.pos(c)
    t.expect(y, "at the shaft's foot").near(foot.y + 0.25, 0.4)
    -- along the gallery to the junction and down to the chain's cavern
    local chain = d.nodes.chain
    t.place(d.nodes.j1.x + 3, d.nodes.j1.y + 0.2, d.nodes.j1.z)
    t.look(-math.pi / 2)
    t.input.hold("move_forward", 120)
    t.wait_until(function() return t.in_zone(c, "chain") end, 120)
    t.input.release("move_forward")
    t.expect(prts.entity.find("caldrith.chain") ~= nil, "Caldrith's chain runs down into the cavern").is_true()
    -- the Wildborn can sink one too, at tier 4
    S.side(2).faction = "wildborn" -- a stand-in side for the check
    local ok = B.place(1, "crown.deep_shaft", -320, -150)
    t.expect(ok, "below tier 4 the Crown cannot sink another").equals(nil)
    S.side(1).mandate_tier = 4
    S.side(1).gold, S.side(1).timber = 2000, 2000
    t.expect(B.place(1, "crown.deep_shaft", -320, -150) ~= nil, "at tier 4 it can").is_true()
    t.expect(require("rts.defs").structure("wildborn.deep_shaft").tier, "the clans' shaft").equals(4)
  end,
}
