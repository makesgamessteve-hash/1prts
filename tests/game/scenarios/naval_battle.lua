-- Spec (water-and-ships): A broadside. A war galleon within range of an enemy longship fires its
-- guns and damages it; a ram galley rams. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "a broadside, and a ram",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local Fl = require("rts.fleet")
    local galleon = prts.entity.find("ship.galleon")
    local gx, _, gz = S.pos(galleon)
    local long = Fl.spawn(2, "wildborn.longship", gx - 200, gz, -math.pi / 2, "ship.test_long")
    local hp = S.data(long).hp
    local fired = 0
    prts.events.on("rts_broadside", "test.naval", function(e) if e == galleon then fired = fired + 1 end end)
    Fl.order(long, gx - 60, gz)
    t.wait_until(function() return fired > 0 end, 90)
    t.ticks(5)
    t.expect(S.data(long).hp < hp, "the longship is hit").is_true()
    -- a ram galley under way rams a Crown hull (the bay's Wildborn go for the nearest one)
    local ram = Fl.spawn(2, "wildborn.ram_galley", gx, gz + 120, 0, "ship.test_ram")
    local victim, before
    prts.events.on("rts_rammed", "test.naval.ram", function(a, b)
      if a == ram and not victim then victim = b end
    end)
    local hps = {}
    for _, e in ipairs(Fl.ships()) do hps[e] = S.data(e).hp end
    Fl.order(ram, gx, gz - 40)
    t.wait_until(function() return victim ~= nil end, 90)
    local vd = S.data(victim)
    t.expect(vd.side, "a Crown hull").equals(1)
    t.expect(vd.hp < hps[victim], "the ram strikes home").is_true()
    local hull = require("rts.defs").ship(vd.def).hull[1]
    t.expect(vd.rooms[hull].hp < vd.rooms[hull].max, "below the waterline").is_true()
  end,
}
