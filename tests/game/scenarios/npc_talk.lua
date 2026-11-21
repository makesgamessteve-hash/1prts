-- Story: the people of Harrowmark talk. Looking at a guard within reach shows the talk prompt;
-- `use` shows their next line as a subtitle with their name, and the line goes away on its own.
-- A run that starts the map at once (this one) plays no prologue.
return {
  name = "talking to the people of Harrowmark",
  max_seconds = 60,
  run = function(t)
    local subtitles = require("ui.subtitles")
    local story = require("story.harrowmark")
    t.ticks(2)
    t.expect(subtitles.current(), "no prologue when the map starts at once").equals(nil)

    -- the guard at the keep door
    t.place(2.3, 0, 13.2)
    t.face("npc.door_guard", 0, 1.3, 0)
    t.ticks(2)
    t.expect(t.target(), "looking at the guard").equals("npc.door_guard")
    t.expect(t.ui_has("[E] Talk to Pell Durran"), "talk prompt").is_true()
    t.input.press("use")
    t.ticks(2)
    t.expect(t.ui_has("Pell Durran, guard of the keep door: "), "name and title").is_true()
    t.expect(t.ui_has("The Great Keep is yours."), "the first line").is_true()
    t.screenshot("npc_talk_door_guard")
    t.input.press("use")
    t.ticks(2)
    t.expect(subtitles.current().text, "the next line").equals(story.PEOPLE.door_guard.lines[2])
    -- the guard has turned to face the Marshal while they talk, and turns back afterwards
    local guard = prts.entity.find("npc.door_guard")
    t.ticks(20)
    local gx, _, gz = prts.entity.pos(guard)
    local cx, _, cz = t.pos(t.commander())
    local want = math.atan(-(cx - gx), -(cz - gz))
    local d = (prts.entity.yaw(guard) - want + math.pi) % (2 * math.pi) - math.pi
    t.expect(math.abs(d) < 0.05, "facing the Marshal").is_true()
    t.expect(t.ui_has("Pell Durran: "), "by name").is_true()
    t.wait_until(function() return subtitles.current() == nil end, 10)
    t.wait_until(function() return not prts.entity.moving(guard) end, 2)
    local back = (prts.entity.yaw(guard) - math.pi + math.pi) % (2 * math.pi) - math.pi
    t.expect(math.abs(back) < 1e-3, "back to the post").is_true()
    t.ticks(1) -- the next frame draws no line
    t.expect(t.ui_has("Tam"), "the line is gone").equals(false)

    -- the castellan by his orders in the great hall
    t.place(2.8, 0.2, -6.5)
    t.face("npc.hall_guard", 0, 1.3, 0)
    t.ticks(2)
    t.expect(t.ui_has("[E] Talk to Edric Thorne"), "the castellan").is_true()
    t.input.press("use")
    t.ticks(2)
    t.expect(t.ui_has("Edric Thorne, Castellan of Harrowmark: "), "the castellan speaks").is_true()
    t.screenshot("npc_talk_castellan")

    -- a guard of the garrison, outside the King's Gate
    local x, _, z = t.pos(prts.entity.find("npc.kings_gate_left"))
    t.place(x, 0, z + 1.6)
    t.face("npc.kings_gate_left", 0, 1.3, 0)
    t.ticks(2)
    t.expect(t.ui_has("[E] Talk to Oswin Tarrant"), "the gate guard").is_true()
    t.input.press("use")
    t.ticks(2)
    t.expect(t.ui_has("Oswin Tarrant, guard of the King's Gate: "), "the gate guard speaks").is_true()
    t.screenshot("npc_talk_kings_gate")
  end,
}
