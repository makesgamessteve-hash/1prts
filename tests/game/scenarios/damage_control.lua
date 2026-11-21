-- Spec (water-and-ships): Holed below the waterline; The pumps keep her afloat. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "damage control: flooding, pumps and sinking",
  max_seconds = 400,
  run = function(t)
    for _, e in ipairs(require("rts.fleet").ships()) do -- this test keeps the Wildborn fleet out of the bay
      if require("rts.state").data(e).side == 2 then require("rts.fleet").gone(e) end
    end
    local S = require("rts.state")
    local Fl = require("rts.fleet")
    local Crew = require("lib.crew")
    local a = Fl.spawn(1, "crown.war_galleon", 300, -60, 0, "ship.test_a")
    local b = Fl.spawn(1, "crown.war_galleon", 300, 60, 0, "ship.test_b")
    t.ticks(2)
    -- both holed in the hold; a's pump crew is lost
    Crew.damage(a, "hold", 400)
    Crew.damage(b, "hold", 400)
    Crew.hurt_room(a, "pumps", 99)
    t.expect(#Crew.living(a, "pumps"), "no one at a's pumps").equals(0)
    t.expect(#Crew.living(b, "pumps") > 0, "b's pumps are manned").is_true()
    local _, y0 = S.pos(a)
    t.ticks(30 * 60)
    local _, y1 = S.pos(a)
    t.expect(S.data(a).flood > 0.2, "a floods (" .. S.data(a).flood .. ")").is_true()
    t.expect(y1 < y0 - 0.3, "and settles lower").is_true()
    t.expect(S.data(b).flood < 0.05, "b's pumps hold the water (" .. S.data(b).flood .. ")").is_true()
    local sunk
    prts.events.on("rts_ship_sunk", "test.dc", function(e) if e == a then sunk = true end end)
    t.wait_until(function() return sunk end, 300)
    t.expect(S.alive(a), "a is gone").equals(false)
    t.expect(S.alive(b) and S.data(b).flood < 1, "b is afloat").is_true()
  end,
}
