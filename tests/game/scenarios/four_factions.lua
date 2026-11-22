-- Spec (brine-league): Four peoples. A skirmish on Tidesend: the Crown, the Wildborn, the Gloam
-- and the League all build and train. MIT License.
-- ctest-labels: slow
-- ctest-args: --start tidesend
return {
  name = "four peoples",
  max_seconds = 400,
  run = function(t)
    local S = require("rts.state")
    local factions = {}
    for _, side in prts.sorted_pairs(S.match().sides) do factions[side.faction] = true end
    t.expect(factions.crown and factions.wildborn and factions.gloam and factions.brine, "four peoples").is_true()
    t.ticks(30 * 300)
    for id = 2, 4 do
      t.expect(#S.structures(id) >= 4, S.side(id).faction .. " builds").is_true()
      t.expect(#S.units(id) >= 8, S.side(id).faction .. " trains").is_true()
    end
    t.expect(S.side(4).pearls > 50, "the League gathers pearls").is_true()
  end,
}
