-- Spec (citadels): Floating in the aether core. Down the shaft into Caldrith's core the commander
-- hangs in the air: no falling, jump rises, crouch sinks, and up the shaft they step out onto
-- the courtyard again. MIT License.
-- ctest-args: --start caldmere
return {
  name = "floating in the aether core",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Deck = require("lib.deckplan")
    local cal = prts.entity.find("Caldrith")
    S.match().caldmere = nil -- the scenario's own rules (boarders, the end) are off in this test
    prts.entity.place(cal, { 0, 129.5, 0 }, 0)
    t.ticks(30)
    local c = t.commander()
    local floating = nil
    prts.events.on("aether_core", "test.core", function(on) floating = on end)
    -- on the courtyard by the shaft, then into it
    t.place(Deck.to_world(cal, 0, 0.1, 2.5))
    t.ticks(20)
    t.expect(t.parent(c), "on Caldrith").equals("Caldrith")
    t.look(0) -- north, over the shaft
    t.input.hold("move_forward", 2.6)
    t.wait_until(function() return floating == true end, 5)
    -- sink into the core with crouch held
    local _, ly0 = Deck.to_frame(cal, t.pos(c))
    t.input.hold("crouch", 3)
    t.ticks(90)
    local _, ly1 = Deck.to_frame(cal, t.pos(c))
    t.expect(ly0 - ly1, "sank while crouching").near(4.5, 0.5)
    -- let go: no fall at all
    t.ticks(60)
    local _, ly2 = Deck.to_frame(cal, t.pos(c))
    t.expect(ly2, "hangs in the air").near(ly1, 0.05)
    t.expect(floating, "floating").equals(true)
    -- glide across the core and rise with jump
    t.input.hold("jump", 2)
    t.ticks(60)
    local _, ly3 = Deck.to_frame(cal, t.pos(c))
    t.expect(ly3 - ly2, "rose while jumping").near(3, 0.4)
    -- back up the shaft and out onto the courtyard
    t.input.hold("jump", 6)
    t.wait_until(function() return floating == false end, 8)
    t.ticks(30)
    local _, ly4 = Deck.to_frame(cal, t.pos(c))
    t.expect(ly4, "standing on the courtyard again").near(0, 0.1)
  end,
}
