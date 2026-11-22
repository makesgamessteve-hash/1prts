-- Spec (brine-league): The raft-city sails. The commander stands on the Tidehold's council deck
-- as it sails across the bay; its great war table shows the war. MIT License.
-- ctest-args: --start reefholm
return {
  name = "the raft-city sails",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local Th = require("props.tidehold")
    local Tools = require("rts.tools")
    local th = S.hq(2)
    t.expect(S.data(th).def, "the League's headquarters").equals("brine.tidehold")
    -- the commander as the League's (for the test)
    S.side(1).ai, S.side(2).ai = true, false
    t.expect(Tools.furnish(th), "the council table").is_true()
    t.place(Th.council_point(th))
    t.ticks(10)
    t.expect(t.parent(t.commander()), "on the council deck").equals(prts.entity.name(th))
    t.ticks(2)
    t.expect(t.ui_has("Pearls", "war_table"), "the table shows the League's war").is_true()
    local x0, y0, z0 = t.pos(th)
    t.expect(y0, "afloat").near(0.8, 0.01)
    Th.order(th, x0 - 120, z0)
    t.ticks(30 * 60)
    local x1, _, z1 = t.pos(th)
    t.expect(x0 - x1 > 40, "sailing west").is_true()
    t.expect(t.parent(t.commander()), "with the commander aboard").equals(prts.entity.name(th))
    local fp = S.data(th).fp
    t.expect(fp[1] < x1 and fp[3] > x1, "its footprint goes with it").is_true()
  end,
}
