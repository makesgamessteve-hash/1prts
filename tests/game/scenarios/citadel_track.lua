-- Spec (citadels): A citadel drifts on its track. Caldrith, raised and left dormant over the
-- Caldmere with a short track, drifts from point to point and back to the first. MIT License.
-- ctest-args: --start caldmere
return {
  name = "a citadel drifts on its track",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local cal = prts.entity.find("Caldrith")
    t.expect(S.data(cal).state, "rising out of the lake").equals("rising")
    local _, y0 = t.pos(cal)
    t.ticks(60)
    local _, y1 = t.pos(cal)
    t.expect(y1 - y0, "climbing slowly").near(Ci.RISE * 2, 0.1)
    -- raised for the test, with a short track over the lake
    prts.entity.place(cal, { 0, 129, 0 }, 0)
    t.wait_until(function() return S.data(cal).state == "dormant" end, 5)
    local d = S.data(cal)
    d.track = { { 0, -30 }, { 30, -30 }, { 0, 0 } }
    d.leg = 1
    local legs = {}
    prts.events.on("citadel_leg", "test.track", function(_, leg) legs[#legs + 1] = leg end)
    t.wait_until(function() return #legs >= 3 end, 170)
    t.expect(legs[1], "on to the second point").equals(2)
    t.expect(legs[3], "and round to the first again").equals(1)
    local x, y, z = t.pos(cal)
    t.expect(math.abs(x) + math.abs(z) < 2, "back where the track begins").is_true()
    t.expect(y, "at its height").near(130, 0.1)
    -- whoever stands in the courtyard rides along
    local Deck = require("lib.deckplan")
    d.leg = 1
    t.place(Deck.to_world(cal, 8, 0.1, -10))
    t.ticks(20)
    t.expect(t.parent(t.commander()), "standing on Caldrith").equals("Caldrith")
    local cx0, _, cz0 = t.pos(t.commander())
    t.ticks(150)
    local cx1, _, cz1 = t.pos(t.commander())
    t.expect(math.abs(cz1 - cz0) > 4, "carried along").is_true()
    t.expect(Ci.aboard(cal), "aboard").is_true()
  end,
}
