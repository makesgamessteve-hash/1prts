-- Spec (under-water): Down in the bell. The commander walks out along the jetty into the diving
-- bell, pulls its lever and goes down with it to the seabed, breathing its air, where its shell
-- table shows the war. MIT License.
-- ctest-args: --start reefholm
return {
  name = "down in the diving bell",
  max_seconds = 120,
  run = function(t)
    local Bell = require("props.bell")
    local Wa = require("rts.water")
    local Tools = require("rts.tools")
    local Deck = require("lib.deckplan")
    local bell = prts.entity.find("bell")
    local c = t.commander()
    t.expect(Bell.furnish(bell), "the shell table").is_true()
    t.place(Deck.to_world(bell, 0, 0.1, 1.6))
    t.ticks(10)
    t.expect(t.parent(c), "in the bell").equals("bell")
    t.face("bell.lever")
    t.ticks(2)
    t.expect(t.target(), "its lever").equals("bell.lever")
    t.input.press("use")
    t.wait_until(function() return Bell.at_bottom(bell) end, 40)
    t.ticks(10)
    local x, y, z = t.pos(c)
    t.expect(t.parent(c), "down with it").equals("bell")
    t.expect(y < -8, "on the seabed").is_true()
    t.expect(Wa.under(x, y + 1.6, z), "in air under the water").equals(false)
    local left, full = Wa.breath()
    t.expect(left, "breathing freely").equals(full)
    t.ticks(2)
    t.expect(t.ui_has("Gold", "war_table"), "the table shows the war").is_true()
    t.expect(Tools.table_host(), "the bell is the command post").equals(bell)
    -- and back up
    t.input.press("use")
    t.wait_until(function() local _, by = t.pos(bell) return by > 1 end, 40)
  end,
}
