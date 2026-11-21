-- Spec (wildborn-clans): Crown against Wildborn. A Wildborn AI on Thornmere builds, reaches a
-- higher tier, trains warriors and takes the headquarters of a Crown side whose serfs only
-- gather. MIT License.
-- ctest-labels: slow
-- ctest-args: --start thornmere
return {
  name = "the Wildborn take Thornmere",
  max_seconds = 4000,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local Ti = require("rts.tiers")
    local V = require("maps.thornmere")
    local tier, kinds = 1, {}
    prts.events.on("rts_trained", "test.wb", function(_, side, id) if side == 2 then kinds[id] = true end end)
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z },
                           V.SIDES[2] }, serfs = 5 }
    for _ = 1, 50 do
      t.ticks(1800)
      tier = math.max(tier, Ti.tier(2))
      if not match.running() then break end
    end
    t.expect(match.winner(), "the Wildborn won").equals(2)
    t.expect(tier >= 2, "they reached a higher tier").is_true()
    t.expect(kinds["wildborn.ghorr"] or kinds["wildborn.spearhurler"], "warriors from the war pit").is_true()
    prts.log.info(string.format("Wildborn won at minute %.1f, tier %d", (S.match().ended - S.match().started) / 1800, tier))
  end,
}
