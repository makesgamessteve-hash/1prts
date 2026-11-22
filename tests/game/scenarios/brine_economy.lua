-- Spec (brine-league): Pearl divers at work. On Tidesend the League's pearl divers work the beds
-- off their shore and its pearls grow. MIT License.
-- ctest-args: --start tidesend
return {
  name = "pearl divers at work",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local St = require("rts.structures")
    local E = require("rts.economy")
    local pearls = S.side(4).pearls
    local divers = S.units(4, "brine.pearl_diver")
    t.expect(#divers, "two divers").equals(2)
    t.expect(S.data(divers[1]).task.res, "at the beds").equals("pearls")
    local bed = E.nearest_source("pearls", 330, 330, 300)
    local bx, by, bz = t.pos(bed)
    t.expect(require("rts.water").depth(bx, bz) > 0, "the bed lies under the water").is_true()
    t.wait_until(function() return S.side(4).pearls > pearls end, 180)
    -- the League's halls stand on a shore
    t.expect(select(2, St.place(4, "brine.wharf_hall", -650, 560)), "not inland").equals("needs a shore")
  end,
}
