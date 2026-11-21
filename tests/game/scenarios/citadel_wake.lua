-- Spec (citadels): Waking the citadel; No caster, no waking. The commander flies up to Caldrith
-- on the skyship with the Magister aboard; the stone does not answer before the Magister is
-- aboard, and wakes Caldrith once the skyship has docked. MIT License.
-- ctest-args: --start caldmere
return {
  name = "waking the citadel",
  max_seconds = 400,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Sh = require("props.skyship")
    local Deck = require("lib.deckplan")
    local cal = prts.entity.find("Caldrith")
    local sky = prts.entity.find("skyship")
    S.match().caldmere = nil -- the scenario's own rules (boarders, the end) are off in this test
    prts.entity.place(cal, { 0, 129.5, 0 }, 0)
    t.wait_until(function() return S.data(cal).state == "dormant" end, 5)
    t.expect(t.parent(t.commander()), "the commander on the skyship").equals("skyship")
    -- up through the storm to Caldrith's dock, with the commander on the deck
    local docked = false
    prts.events.on("skyship_docked", "test.wake", function() docked = true end)
    Sh.dock(sky, cal)
    local storm = false
    prts.events.on("storm_fog", "test.wake", function(on) storm = storm or on end)
    t.wait_until(function() return docked end, 300)
    t.expect(storm, "through the storm on the way").is_true()
    t.expect(t.parent(t.commander()), "still aboard").equals("skyship")
    local men, caster = Ci.strength(cal, 1)
    t.expect(men, "the two companies and the Magister are in the garrison").equals(17)
    t.expect(caster, "a caster among them").is_true()
    t.expect(#S.data(sky).cargo, "the skyship's hold empty").equals(0)
    -- across the gangway onto the dock
    local x, y, z = Deck.to_world(sky, 2.6, 0.1, 0)
    t.place(x, y, z)
    t.look(prts.entity.yaw(sky) - math.pi / 2)
    t.input.hold("move_forward", 1.5)
    t.ticks(50)
    t.expect(t.parent(t.commander()), "stepped across onto Caldrith").equals("Caldrith")
    -- the stone needs the Magister: without one the stone stays dark
    local rec = S.data(cal).garrison
    local saved = rec
    S.data(cal).garrison = {}
    local ok, why = Ci.use_stone(cal)
    t.expect(ok, "no caster: no waking").equals(false)
    t.expect(why).equals("needs a caster")
    t.expect(S.data(cal).state, "still dormant").equals("dormant")
    S.data(cal).garrison = saved
    -- down into the core, to the stone, and hands on it
    t.place(Deck.to_world(cal, 0, -9.4, 0.9))
    t.ticks(5)
    t.face("Caldrith.aetherstone")
    t.ticks(2)
    t.expect(t.target(), "the aetherstone in reach").equals("Caldrith.aetherstone")
    local aether = S.side(1).aether
    t.input.press("use")
    t.ticks(2)
    t.expect(S.side(1).aether, "the stone drinks aether").equals(aether - Ci.WAKE_AETHER)
    t.expect(S.data(cal).waking ~= nil, "the ritual has begun").is_true()
    local woken = false
    prts.events.on("citadel_woken", "test.wake", function(_, side) woken = side end)
    t.wait_until(function() return woken end, Ci.WAKE_SECONDS + 3)
    t.expect(woken, "woken for the Crown").equals(1)
    t.expect(S.data(cal).state).equals("woken")
    t.expect(require("rts.command").state().table.host, "the throne hall's table is the command post").equals(cal)
    t.expect(Ci.use_stone(cal), "it cannot be woken twice").equals(false)
  end,
}
