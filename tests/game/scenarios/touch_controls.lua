-- Spec (input-actions): Touch movement and look (both halves at the same time), tap to use,
-- context buttons that show only when usable, and Tablet with a keyboard.
return {
  name = "touch: move and look at the same time, tap to use",
  run = function(t)
    local c = t.commander()
    t.place("start")
    t.ticks(2)
    local x0, _, z0 = t.pos(c)
    local yaw0 = prts.commander.view()
    -- one finger on the left (the stick), one on the right (look), at the same time
    t.touch(1, 0.25, 0.70, true)
    t.touch(2, 0.75, 0.50, true)
    t.ticks(1)
    t.expect(prts.input.scheme(), "touch scheme").equals("touch")
    for i = 1, 45 do
      t.touch(1, 0.25, 0.58, true)               -- stick pushed up: forward
      t.touch(2, 0.75 + i * 0.003, 0.50, true)   -- dragging right: turn right
      t.ticks(1)
    end
    local x1, _, z1 = t.pos(c)
    local yaw1 = prts.commander.view()
    t.expect(math.sqrt((x1 - x0) ^ 2 + (z1 - z0) ^ 2) > 0.8, "the commander moved").is_true()
    t.expect(yaw1 < yaw0 - 0.1, "the view turned right").is_true()
    t.expect(t.ui_has("Jump"), "context buttons are drawn").is_true()
    t.expect(t.ui_has("Use"), "nothing to use here").equals(false)
    t.touch(1, 0.25, 0.58, false)
    t.touch(2, 0.885, 0.50, false)
    t.ticks(2)

    -- a tap on the right half uses what the commander looks at
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    t.expect(t.ui_has("Use"), "the use button shows when usable").is_true()
    t.touch(3, 0.70, 0.30, true)
    t.ticks(2)
    t.touch(3, 0.70, 0.30, false)
    t.ticks(2)
    t.expect(prts.entity.data(prts.entity.find("keep.door")).open, "tap opened the door").is_true()

    -- Tablet with a keyboard: a movement key switches back to keyboard and mouse
    t._key("W", true)
    t.ticks(2)
    t._key("W", false)
    t.ticks(1)
    t.expect(prts.input.scheme(), "keyboard scheme").equals("kbm")
    t.expect(t.ui_has("Jump"), "touch overlay hidden").equals(false)
  end,
}
