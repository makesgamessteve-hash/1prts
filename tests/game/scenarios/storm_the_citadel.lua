-- Spec (storm-the-citadel): Storming Caldrith. Caldrith rises out of the Caldmere; the commander
-- flies up through the storm on the skyship with two companies and a Magister, docks, the
-- garrison beats the clan's boarders, and the commander wakes Caldrith with Ashby. MIT License.
-- ctest-labels: slow
-- ctest-args: --start caldmere
return {
  name = "Storm the Citadel",
  max_seconds = 900,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Sh = require("props.skyship")
    local Cmd = require("rts.command")
    local Deck = require("lib.deckplan")
    local match = require("rts.match")
    local cal = prts.entity.find("Caldrith")
    local sky = prts.entity.find("skyship")
    t.ticks(5)
    t.expect(t.parent(t.commander()), "on the skyship's deck").equals("skyship")
    -- the skyship is chosen on the field map and sent to dock at Caldrith
    local sx, _, sz = t.pos(sky)
    t.expect(Cmd.point(sx, sz), "the skyship chosen").equals("ship")
    t.expect(Cmd.point(0, 0), "sent to dock at Caldrith").equals("dock")
    local risen, docked, boarded = false, false, 0
    prts.events.on("citadel_risen", "test.storm", function() risen = true end)
    prts.events.on("skyship_docked", "test.storm", function() docked = true end)
    prts.events.on("rts_boarded", "test.storm", function() boarded = boarded + 1 end)
    t.wait_until(function() return risen end, 200)
    t.wait_until(function() return docked end, 200)
    local _, y = t.pos(cal)
    t.expect(y, "Caldrith above the clouds").near(130, 0.1)
    t.expect(Ci.strength(cal, 1), "the relief in the garrison").equals(17)
    -- across onto the dock: the commander's banner aboard
    local x, cy, z = Deck.to_world(sky, 2.6, 0.1, 0)
    t.place(x, cy, z)
    t.look(prts.entity.yaw(sky) - math.pi / 2)
    t.input.hold("move_forward", 2)
    t.ticks(60)
    t.expect(t.parent(t.commander()), "aboard Caldrith").equals("Caldrith")
    -- the drakes come up, and the boarders are beaten in the halls
    t.wait_until(function() return boarded >= 2 end, 120)
    t.wait_until(function() return Ci.strength(cal, 2) == 0 end, 90)
    t.expect(Ci.strength(cal, 1) > 0, "the Crown holds the halls").is_true()
    local _, caster = Ci.strength(cal, 1)
    t.expect(caster, "the Magister lives").is_true()
    -- down to Ashby at the stone
    t.place(Deck.to_world(cal, 1.5, -9.4, 1.5))
    t.ticks(5)
    t.face("ashby")
    t.ticks(2)
    t.expect(t.target(), "Ashby at the stone").equals("ashby")
    t.input.press("use")
    t.ticks(2)
    t.face("Caldrith.aetherstone")
    t.ticks(2)
    t.input.press("use")
    t.ticks(2)
    t.expect(S.data(cal).waking ~= nil, "the ritual has begun").is_true()
    t.wait_until(function() return not match.running() end, Ci.WAKE_SECONDS + 10)
    t.expect(S.data(cal).state, "Caldrith woken").equals("woken")
    t.expect(match.winner(), "the Crown wins").equals(1)
  end,
}
