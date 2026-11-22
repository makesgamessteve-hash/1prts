-- Spec (under-the-hill): Taking the Hollow Court. The Crown's foot and bows go down through the
-- gate under Merewatch Hill, along the tunnel road past Caldrith's chain, come up at the Hollow
-- Court's ramp and bring the Court down. MIT License.
-- ctest-labels: slow
-- ctest-args: --start underhill
return {
  name = "Under the Hill",
  max_seconds = 900,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local match = require("rts.match")
    local up = "hall.302.2.up"
    S.match().underhill = nil -- the wyrm stays home in this test (tunnel_wyrm.lua sends it)
    S.side(2).ai = false      -- and the Court keeps its guard at home
    local out = 0
    prts.events.on("rts_tunnel_out", "test.hill", function(_, side) if side == 1 then out = out + 1 end end)
    for _, c in ipairs(C.of_side(1)) do t.expect(Tu.travel(c, "west", up), c.name .. " into the tunnels").is_true() end
    t.wait_until(function() return out >= 12 end, 600)
    -- up at the Court: attack its gatehouse
    local court = S.hq(2)
    local cx, _, cz = t.pos(court)
    for _, c in ipairs(C.of_side(1)) do
      if #C.members(c) > 0 then C.order(c, { kind = "attack", x = cx, z = cz }) end
    end
    S.data(court).hp = 400 -- (the test spares the long siege)
    t.wait_until(function() return not match.running() end, 240)
    t.expect(match.winner(), "the Crown takes the Hollow Court").equals(1)
  end,
}
