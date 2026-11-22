-- Spec (touch-polish): Left-handed with large buttons; Buttons under the water. On the
-- Saltcoast: the touch buttons at 1.5 times their size, mirrored onto the left, the stick on the
-- right half; swimming, the jump and crouch buttons read Rise and Dive. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "touch for either hand, and under the water",
  max_seconds = 60,
  run = function(t)
    local settings = require("game.settings")
    local T = require("ui.touch")
    local c = t.commander()
    t.place(150, prts.scene.height(150, -330) + 0.1, -330)
    t.ticks(5)
    t.touch(1, 0.5, 0.5, true)
    t.ticks(1)
    t.touch(1, 0.5, 0.5, false)
    t.ticks(1)
    t.expect(prts.input.scheme(), "touch").equals("touch")
    local before = prts.input.touch_state().buttons
    settings.set_touch("scale", 1.5, false)
    settings.set_touch("left_handed", true, false)
    local after = prts.input.touch_state().buttons
    for i, b in ipairs(after) do
      t.expect(b.radius, b.id .. " half again as large").near(before[i].radius * 1.5, 1e-4)
      t.expect(b.x, b.id .. " mirrored").near(1 - before[i].x, 1e-4)
    end
    -- the stick on the right half now: a finger there walks
    local x0, _, z0 = t.pos(c)
    t.touch(1, 0.75, 0.70, true)
    t.ticks(1)
    for _ = 1, 40 do
      t.touch(1, 0.75, 0.58, true)
      t.ticks(1)
    end
    t.touch(1, 0.75, 0.58, false)
    local x1, _, z1 = t.pos(c)
    t.expect(math.sqrt((x1 - x0) ^ 2 + (z1 - z0) ^ 2) > 0.8, "the right-hand stick walks").is_true()
    -- in deep water: Rise and Dive
    t.expect(T.label("jump"), "on land").equals("Jump")
    t.place(150, -1.1, 0)
    t.ticks(10)
    t.expect(T.label("jump"), "swimming: rise").equals("Rise")
    t.expect(T.label("crouch"), "swimming: dive").equals("Dive")
    t.ticks(2)
    t.expect(t.ui_has("Rise") and t.ui_has("Dive"), "the buttons say so").is_true()
    t.expect(T.label("mount"), "nothing to dismount").equals("Ride")
    t.expect(T.label("nothing"), "an unknown button").equals("nothing")
    settings.set_touch("scale", 1.0, false)
    settings.set_touch("left_handed", false, false)
  end,
}
