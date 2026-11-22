-- Spec (gloam-courts): Down to the war room. Leading the Hollow Court, the commander walks down
-- the stair-ramp from beside the gatehouse into the hall under the ground, where the Court's war
-- table shows the war. Its stages are Delve, Hollow Court and Sunken Throne. MIT License.
-- ctest-args: --start underhill
return {
  name = "down to the Gloam war room",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local Tu = require("rts.tunnels")
    local Tools = require("rts.tools")
    -- the commander leads the Court for this test
    S.side(1).ai, S.side(2).ai = true, false
    local hq = S.hq(2)
    t.expect(S.data(hq).def, "the Hollow Court").equals("gloam.hollow_court")
    t.expect(D.structure("gloam.delve").next, "from the Delve").equals("gloam.hollow_court")
    t.expect(D.structure("gloam.hollow_court").next, "to the Sunken Throne").equals("gloam.sunken_throne")
    S.data(hq).furnished = nil
    t.expect(Tools.furnish(hq), "the table set in the hall").is_true()
    local d = Tu.data()
    local hall, up = d.nodes["hall.302.2"], d.nodes["hall.302.2.up"]
    -- down the ramp from the field, along the stair gallery into the hall
    t.place(up.top[1], up.top_y + 0.2, up.top[2])
    t.look(math.pi) -- south, down the ramp
    t.input.hold("move_forward", 90)
    t.wait_until(function() local _, _, z = t.pos(t.commander()) return z > hall.z - 6 end, 80)
    t.input.release("move_forward")
    local x, y, z = t.pos(t.commander())
    t.expect(y, "on the hall's floor, under the ground").near(hall.y, 0.4)
    t.expect(Tu.underground(x, y + 1.6, z), "under the ground").is_true()
    t.ticks(5)
    t.expect(t.ui_has("Gold", "war_table"), "the war table shows the war").is_true()
    t.expect(t.ui_has("Stone", "war_table"), "with the Court's stone").is_true()
    local tx, ty, tz = Tools.table_point(0.5, 0.5)
    t.expect(ty, "the table stands in the hall").near(hall.y + Tools.TABLE.top, 0.2)
    t.expect(S.dist(tx, tz, x, z) < 12, "a few steps on").is_true()
  end,
}
