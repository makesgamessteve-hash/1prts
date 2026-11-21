-- Spec (storm-the-citadel): A citadel over Thornmere. A skirmish on Thornmere has Vael Tor
-- drifting on its track high over the field, above the cloud deck. MIT License.
-- ctest-args: --start thornmere
return {
  name = "a citadel over Thornmere",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local map = require("maps.thornmere")
    local all = Ci.all()
    t.expect(#all, "one citadel").equals(1)
    local vt = all[1]
    t.expect(prts.entity.name(vt)).equals("Vael Tor")
    t.expect(S.data(vt).state, "dormant").equals("dormant")
    local _, y = t.pos(vt)
    t.expect(y > require("rts.storm").high(), "above the clouds").is_true()
    local x0, _, z0 = t.pos(vt)
    t.ticks(30 * 20)
    local x1, _, z1 = t.pos(vt)
    t.expect(z1 < z0 - 15, "drifting towards the first point of its track").is_true()
    t.expect(math.abs(x1 - map.CITADEL.track[1][1]) < 10, "along the track").is_true()
    -- a side's war table shows it passing
    t.expect(require("rts.match").running(), "the skirmish goes on beneath it").is_true()
  end,
}
