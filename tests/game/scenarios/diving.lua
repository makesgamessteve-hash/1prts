-- Spec (under-water): Diving to the reef; Out of breath. Off the Crown's jetty at Reefholm the
-- commander swims out, dives with crouch into the blue-green dark, rises with jump, and stays
-- down past their breath until they choke. MIT License.
-- ctest-args: --start reefholm
return {
  name = "diving",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local Wa = require("rts.water")
    local Fog = require("lib.fog")
    local c = t.commander()
    local fogged, swimming = nil, nil
    prts.events.on("underwater", "test.dive", function(on) fogged = on end)
    prts.events.on("swimming", "test.dive", function(on) swimming = on end)
    -- in deep water off the jetty, at the surface
    local x, z = -40, -300
    t.expect(Wa.depth(x, z) > 8, "deep water").is_true()
    t.place(x, -Wa.WADE, z)
    t.ticks(10)
    t.expect(swimming, "swimming").equals(true)
    local _, y0 = t.pos(c)
    t.expect(y0, "treading water at the surface").near(-Wa.WADE, 0.1)
    -- crouch: down towards the seabed, into the blue-green
    t.input.hold("crouch", 4)
    t.ticks(120)
    local _, y1 = t.pos(c)
    t.expect(y0 - y1, "diving").near(Wa.SWIM.dive * 4, 0.6)
    t.expect(fogged, "the fog closes in").equals(true)
    local near, far, col, layer = Fog.current()
    t.expect(layer, "the underwater fog").equals("underwater")
    t.expect(far, "blue-green and close").near(Wa.FOG.far, 0.01)
    -- glide forward a little, then up again with jump
    t.input.hold("move_forward", 1)
    t.ticks(30)
    t.input.hold("jump", 3)
    t.ticks(90)
    local _, y2 = t.pos(c)
    t.expect(y2 > y1 + 2.5, "rising").is_true()
    -- stay down past the breath: choking hurts
    local left, full = Wa.breath()
    t.expect(left < full, "breath used").is_true()
    t.expect(full, "thirty seconds of breath").equals(Wa.BREATH)
    t.input.hold("crouch", 3)
    t.ticks(100)
    local hurt = 0
    prts.events.on("commander_hurt", "test.dive", function() hurt = hurt + 1 end)
    t.wait_until(function() return hurt > 0 end, Wa.BREATH + 5)
    t.expect(select(1, Wa.breath()), "no breath left").equals(0)
    -- up to the surface: breath comes back
    t.input.hold("jump", 12)
    t.wait_until(function() return fogged == false end, 15)
    t.wait_until(function() local l, f = Wa.breath() return l >= f end, 10)
    -- a League commander's breath charm lasts four times as long
    S.side(1).faction = "brine"
    t.expect(Wa.lungs(), "a breath charm").equals(Wa.BREATH * Wa.CHARM)
    S.side(1).faction = "crown"
  end,
}
