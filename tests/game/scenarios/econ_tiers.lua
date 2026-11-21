-- Spec (economy-and-tiers): Upgrading the holdfast to a keep; A mandate needs its buildings;
-- Tempered blades. MIT License.
-- ctest-args: --start thornmere
return {
  name = "tiers, mandates and upgrades",
  max_seconds = 600,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local D = require("rts.defs")
    local Ti = require("rts.tiers")
    local Cmd = require("rts.command")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z,
                             gold = 5000, timber = 5000, aether = 1000 },
                           { id = 2, name = "Clan", faction = "wildborn", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local side = S.side(1)
    t.expect(Ti.tier(1), "tier 1").equals(1)
    t.expect(select(2, B.place(1, "crown.forgeworks", V.WEST.x + 70, V.WEST.z + 60)), "the Forgeworks needs tier 2")
      .equals("needs tier 2")
    local gold = side.gold
    t.expect(Cmd.work("hq_up"), "the keep is ordered").is_true()
    t.expect(gold - side.gold, "and paid for").equals(D.structure("crown.keep").cost.gold)
    t.expect(select(2, Cmd.work("hq_up")), "once").equals("already rising")
    t.wait_until(function() return S.hq(1) and S.data(S.hq(1)).def == "crown.keep" end, D.structure("crown.keep").build + 5)
    t.expect(Ti.tier(1), "tier 2").equals(2)
    t.expect(B.place(1, "crown.forgeworks", V.WEST.x + 70, V.WEST.z + 60) ~= nil, "now the Forgeworks can be built").is_true()
    -- the high castle, then the charter of mastery: refused without its buildings
    t.expect(Cmd.work("hq_up")).is_true()
    t.wait_until(function() return Ti.tier(1) == 3 end, D.structure("crown.high_castle").build + 5)
    t.expect(select(2, Ti.buy_mandate(1, "crown.charter_of_mastery")), "needs its buildings")
      .equals("needs crown.smithy")
    B.place(1, "crown.smithy", V.WEST.x - 70, V.WEST.z + 60, true)
    B.place(1, "crown.forgeworks", V.WEST.x - 70, V.WEST.z - 70, true)
    t.expect(Ti.buy_mandate(1, "crown.charter_of_mastery"), "granted").is_true()
    t.expect(Ti.tier(1), "tier 4").equals(4)
    t.expect(select(2, Ti.buy_mandate(1, "crown.charter_of_mastery")), "only once").equals("already granted")
    t.expect(select(2, Ti.buy_mandate(1, "crown.charter_of_dominion")), "dominion needs a harbour")
      .equals("needs crown.harbor")
    t.expect(select(2, Ti.buy_mandate(1, "wildborn.rite_of_mastery")), "not the Crown's").equals("unknown mandate")
    -- tempered blades at the smithy
    local u = D.unit("crown.men_at_arms")
    t.expect(Ti.damage_bonus(1, "crown.men_at_arms"), "no bonus yet").equals(0)
    t.expect(Cmd.work("research:crown.tempered_blades"), "researching").is_true()
    t.expect(select(2, Cmd.work("research:crown.tempered_blades")), "once").equals("already known")
    t.wait_until(function() return side.upgrades["crown.tempered_blades"] end, 60)
    t.expect(Ti.damage_bonus(1, "crown.men_at_arms"), "men-at-arms strike harder").equals(3)
    t.expect(Ti.damage_bonus(1, "crown.crossbowmen"), "not the bows").equals(0)
    -- and they really do: a blow on a man of the other side
    local foot = S.new_company(1, "Foot")
    local a = S.spawn_unit(1, "crown.men_at_arms", 0, 40, foot)
    local b = S.spawn_unit(2, "wildborn.ghorr", 1.5, 40)
    local hp = S.data(b).hp
    t.ticks(60)
    local lost = hp - S.data(b).hp
    t.expect(lost >= u.weapon.damage + 3 - D.unit("wildborn.ghorr").armor, "a blow with tempered blades (" .. lost .. ")").is_true()
    t.expect(S.alive(a)).is_true()
  end,
}
