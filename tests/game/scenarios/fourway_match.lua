-- Spec (gloam-courts): Four sides, three factions. On Hollowfell the Crown, Clan Varrak, a rival
-- Crown host and the Hollow Court all build and train; sides drop out as their headquarters
-- fall, and the last one standing wins. MIT License.
-- ctest-labels: slow
-- ctest-args: --start hollowfell
return {
  name = "a four-way match",
  max_seconds = 400,
  run = function(t)
    local S = require("rts.state")
    local match = require("rts.match")
    local B = require("rts.structures")
    t.expect(#S.enemies(1), "everyone against everyone").equals(3)
    t.expect(prts.scene.hole_at(2, 2), "the sinkhole over the cavern").is_true()
    local factions = {}
    for _, side in prts.sorted_pairs(S.match().sides) do factions[side.faction] = true end
    t.expect(factions.crown and factions.wildborn and factions.gloam, "three peoples").is_true()
    local foe, id = S.foe(4, 542, 542)
    t.expect(foe ~= nil and id ~= 4, "the Court picks a foe").is_true()
    t.ticks(30 * 240)
    for id = 2, 4 do
      t.expect(#S.structures(id) >= 4, "side " .. id .. " builds").is_true()
      t.expect(#S.units(id) >= 8, "side " .. id .. " trains").is_true()
    end
    t.expect(S.side(4).stone ~= nil, "the Court gathers stone").is_true()
    -- the clans and the rival host fall: the match goes on between the last two
    local function raze(id)
      for _, hq in ipairs(S.hqs(id)) do B.damage(hq, 1e7) end
    end
    raze(2)
    raze(3)
    t.ticks(31)
    t.expect(S.side(2).lost and S.side(3).lost, "two sides out").is_true()
    t.expect(match.running(), "the fight goes on").is_true()
    t.expect(#S.enemies(1), "one foe left").equals(1)
    raze(4)
    t.ticks(31)
    t.expect(match.running(), "over").equals(false)
    t.expect(match.winner(), "the Crown stands last").equals(1)
  end,
}
