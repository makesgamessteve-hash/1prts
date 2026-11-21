-- Spec (storm-the-citadel): Caldrith broken. Nobody comes to Caldrith's aid: the clan's boarders
-- hold its core alone until the aetherstone breaks, and the Crown loses. MIT License.
-- ctest-args: --start caldmere
return {
  name = "Caldrith broken",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local match = require("rts.match")
    local cal = prts.entity.find("Caldrith")
    prts.entity.place(cal, { 0, 129.5, 0 }, 0) -- risen already (the test skips the climb)
    local broken = false
    prts.events.on("citadel_broken", "test.lost", function() broken = true end)
    t.wait_until(function() return Ci.strength(cal, 2) > 0 end, 120)
    t.wait_until(function() return broken end, Ci.BREAK_SECONDS + 10)
    t.wait_until(function() return not match.running() end, 3)
    t.expect(match.winner(), "Clan Varrak wins").equals(2)
    t.expect(S.data(cal).state).equals("broken")
    local _, y0 = t.pos(cal)
    t.ticks(90)
    local _, y1 = t.pos(cal)
    t.expect(y0 - y1, "it falls").near(Ci.RISE * 3, 0.1)
  end,
}
