-- Spec (input-actions): Prompt update (scheme-changed event, prompt shows the gamepad button).
-- Spec (commander-locomotion): Using objects (an interaction prompt appears when usable).
return {
  name = "interaction prompt shows the glyph of the active input scheme",
  run = function(t)
    local schemes = {}
    prts.events.on("input_scheme", "test.scenario", function(s) schemes[#schemes + 1] = s end)
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    t.expect(t.ui_has("[E] Open door"), "keyboard prompt").is_true()

    t.scheme("gamepad")
    t.ticks(2)
    t.expect(schemes[#schemes], "scheme-changed event").equals("gamepad")
    t.expect(t.ui_has("(X) Open door"), "gamepad prompt").is_true()
    t.expect(t.ui_has("[E]"), "keyboard glyph gone").equals(false)

    t.scheme("touch")
    t.ticks(2)
    t.expect(t.ui_has("Tap: Open door"), "touch prompt").is_true()

    t.scheme("kbm")
    t.ticks(2)
    t.expect(t.ui_has("[E] Open door"), "back to the keyboard glyph").is_true()
    t.input.press("use")
    t.ticks(30) -- the door swings out of view: nothing to use, no prompt
    t.expect(prts.entity.data(prts.entity.find("keep.door")).open, "door open").is_true()
    t.expect(t.ui_has("door"), "no prompt").equals(false)
  end,
}
