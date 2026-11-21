-- Spec (high-sky-units): Only at tier six; The sky galleon's guns; Drakes board a citadel.
-- MIT License.
-- ctest-args: --start caldmere
return {
  name = "tier six in the high sky",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local B = require("rts.structures")
    local Sk = require("rts.sky")
    local Ci = require("rts.citadel")
    S.match().caldmere = nil
    S.side(1).gold, S.side(1).timber, S.side(1).aether, S.side(1).powder = 5000, 5000, 5000, 500
    S.side(2).gold, S.side(2).bone, S.side(2).aether = 5000, 1000, 5000
    S.side(2).food_bonus = 50
    -- below tier 6 the orders are refused; at tier 6 they are taken
    local mast = S.structures(1, "crown.sky_mast")[1]
    local ok, why = B.order(mast, "crown.sky_galleon")
    t.expect(ok, "no sky galleon below tier 6").equals(false)
    t.expect(why).equals("needs tier 6")
    local roost = S.structures(2, "wildborn.roost_cliffs")[1]
    ok, why = B.order(roost, "wildborn.elder_drake")
    t.expect(ok, "no elder drake below tier 6").equals(false)
    t.expect(why).equals("needs tier 6")
    S.side(1).mandate_tier, S.side(2).mandate_tier = 6, 6
    t.expect(B.order(mast, "crown.sky_galleon"), "a sky galleon ordered at tier 6").is_true()
    local galleon
    prts.events.on("rts_airship", "test.t6", function(e) galleon = e end)
    B.train_tick(mast, 200)
    t.expect(galleon ~= nil, "the sky galleon is aloft").is_true()
    t.expect(require("props.skyship").kind(galleon).guns ~= nil, "a warship").is_true()
    -- the galleon's guns against wyverns
    local gx, gy, gz = t.pos(galleon)
    local wings = {}
    for _, e in ipairs(Sk.flyers()) do
      if S.data(e).def == "wildborn.wyvern" then wings[#wings + 1] = e end
    end
    local hp = 0
    for i, w in ipairs(wings) do
      local d = S.data(w)
      d.alt, d.goal = gy, { gx + 40, gz + i * 4 }
      prts.entity.place(w, { gx + 40, gy, gz + i * 4 })
      hp = hp + d.hp
    end
    local fired = false
    prts.events.on("skyship_broadside", "test.t6", function(_, what) fired = fired or what == "flyer" end)
    t.wait_until(function() return fired end, 6)
    local now = 0
    for _, w in ipairs(wings) do now = now + (prts.entity.alive(w) and S.data(w).hp or 0) end
    t.expect(now < hp, "the wyverns are hit").is_true()
    -- drakes board Caldrith once it hangs over the lake
    local cal = prts.entity.find("Caldrith")
    prts.entity.place(cal, { 0, 129.5, 0 }, 0)
    t.ticks(30)
    local n = 0
    for _, e in ipairs(Sk.flyers()) do
      if S.data(e).def == "wildborn.elder_drake" and Sk.board(e, cal) then n = n + 1 end
    end
    t.expect(n, "two drakes sent").equals(2)
    local boarded = 0
    prts.events.on("rts_boarded", "test.t6", function() boarded = boarded + 1 end)
    t.wait_until(function() return boarded == 2 end, 150)
    t.expect(Ci.strength(cal, 2), "twelve warriors in the garrison").equals(12)
    -- wyverns cannot board
    local wyv = S.spawn_unit(2, "wildborn.wyvern", 0, 300, S.new_company(2, "Wyverns"))
    t.expect(Sk.board(wyv, cal), "no wyvern boards").equals(false)
  end,
}
