-- Spec (brine-league): Harpoons and hull-borers; A brigantine from the yard. MIT License.
-- ctest-args: --start reefholm
return {
  name = "the League's soldiers and ships",
  max_seconds = 150,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local K = require("rts.combat")
    local St = require("rts.structures")
    local Fl = require("rts.fleet")
    S.side(2).ai = true
    local function one(side, unit, x, z) return S.spawn_unit(side, unit, x, z, S.new_company(side, "T")) end
    -- a harpoon pins a rider
    local harp = one(2, "brine.harpooneers", -300, -560)
    local rider = one(1, "crown.knight", -290, -560)
    local speed = prts.agent.info(rider).speed
    local hd = S.data(harp)
    hd.target, hd.cd = rider, 0
    K.think(harp, 6)
    t.expect(prts.agent.info(rider).speed, "pinned: half speed").near(speed * 0.5, 0.01)
    prts.agent.remove(harp)
    prts.entity.despawn(harp) -- (no second harpoon)
    t.ticks(30 * 7)
    t.expect(prts.agent.info(rider).speed, "free again").near(speed, 0.01)
    -- hull-borers on the shore strike a ship lying off it, four times as hard
    local galleon = prts.entity.find("ship.galleon")
    local gx, _, gz = t.pos(galleon)
    local borer = one(2, "brine.hull_borer", -10, -420)
    local bx, _, bz = t.pos(borer)
    prts.entity.place(galleon, { bx, 0, bz + 9 + D.ship("crown.war_galleon").beam / 2 }, math.pi / 2)
    local hp = S.data(galleon).hp
    S.data(borer).cd = 0
    t.expect(K.think(borer, 6), "at the hull").equals("fighting")
    t.expect(hp - S.data(galleon).hp, "a hole bored").near(D.unit("brine.hull_borer").weapon.damage * 4, 0.01)
    -- a brigantine from a shipwright yard on the shore
    S.side(2).gold, S.side(2).timber, S.side(2).mandate_tier = 5000, 5000, 3
    local yard = St.place(2, "brine.shipwright_yard", 560, 70, true)
    S.data(yard).seaward = -1
    t.expect(St.order(yard, "brine.brigantine"), "ordered").is_true()
    local launched
    prts.events.on("rts_ship_launched", "test.brine", function(e, _, id) if id == "brine.brigantine" then launched = e end end)
    St.train_tick(yard, 200)
    t.expect(launched ~= nil, "launched").is_true()
    t.expect(#Fl.ships() >= 6, "a sixth hull on the water").is_true()
  end,
}
