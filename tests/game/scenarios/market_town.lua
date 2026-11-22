-- Spec (neutral-places): Trading at the market. The Crown sells timber at Saltwick twice (the
-- second sale fetches less), the League would get better prices, and holding the town brings
-- food and the traders' rumours of the League's men nearby. MIT License.
-- ctest-args: --start reefholm
return {
  name = "trading at the market",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local N = require("rts.neutral")
    local C = require("rts.companies")
    local town = prts.entity.find("Saltwick")
    S.side(1).timber = 500
    local g1 = N.trade(1, town, "timber", 100)
    local g2 = N.trade(1, town, "timber", 100)
    t.expect(g1 > 0 and g2 < g1, "the price falls as timber floods in").is_true()
    t.expect(N.price(town, 2, "timber", 100) > N.price(town, 1, "timber", 100), "the League gets more").is_true()
    local gold = S.side(1).gold
    t.expect(N.trade(1, town, "pearls", -10) > 0, "buying pearls").is_true()
    t.expect(S.side(1).gold < gold and S.side(1).pearls == 10, "paid in gold").is_true()
    t.expect(select(2, N.trade(1, town, "timber", 1e6)), "can't sell what you lack").equals("not enough timber")
    t.expect(select(2, N.trade(1, town, "gold", 10)), "gold is the coin").equals("not traded here")
    -- hold it: food and rumours
    local food = S.food(1)
    local foot = C.of_side(1)[1]
    local tx, _, tz = t.pos(town)
    for _, m in ipairs(C.members(foot)) do prts.entity.place(m, { tx + 3, prts.scene.height(tx + 3, tz), tz + 3 }) end
    local held = nil
    prts.events.on("rts_place_held", "test.market", function(_, side) held = side end)
    t.wait_until(function() return held == 1 end, N.HOLD_SECONDS + 3)
    t.expect(S.food(1), "the town feeds the Crown").equals(food + N.FOOD)
    local spy = S.spawn_unit(2, "brine.marines", tx - 200, tz, S.new_company(2, "Spies"))
    S.side(1).picture[spy] = nil
    t.wait_until(function() return S.side(1).picture[spy] ~= nil end, N.RUMOR.every + 2)
    t.expect(require("rts.picture").state(S.side(1).picture[spy], t.tick()), "a rumour, not a sighting").equals("old")
  end,
}
