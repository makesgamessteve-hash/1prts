-- Spec (under-water): The sky-barge's relic. The commander dives to the sunken sky-barge, catches
-- their breath in its after-hold and opens the relic chest: aether for the Crown, once. MIT License.
-- ctest-args: --start reefholm
return {
  name = "the sky-barge's relic",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Wa = require("rts.water")
    local Deck = require("lib.deckplan")
    local wreck = prts.entity.find("sky-barge")
    local c = t.commander()
    -- swimming down by the wreck, short of breath
    local x, y, z = Deck.to_world(wreck, 0, 2, 6)
    t.place(x, y, z)
    t.ticks(30 * 20)
    local left = Wa.breath()
    t.expect(left < Wa.BREATH - 15, "breath running out").is_true()
    -- into the after-hold: air
    t.place(Deck.to_world(wreck, 0, 1.3, 15))
    t.ticks(10)
    local px, py, pz = t.pos(c)
    t.expect(Wa.in_air(px, py + 1, pz), "in the trapped air").is_true()
    t.wait_until(function() local l, f = Wa.breath() return l >= f end, 5)
    -- the relic
    t.face("sky-barge.relic")
    t.ticks(2)
    t.expect(t.target(), "the relic chest").equals("sky-barge.relic")
    local aether = S.side(1).aether
    t.input.press("use")
    t.ticks(2)
    t.expect(S.side(1).aether, "aether from the relic").equals(aether + require("props.wreck").RELIC)
    t.input.press("use")
    t.ticks(2)
    t.expect(S.side(1).aether, "only once").equals(aether + require("props.wreck").RELIC)
  end,
}
