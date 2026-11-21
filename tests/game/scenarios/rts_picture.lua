-- Spec (rts-core): An enemy seen, then lost. An enemy squad walks into a friendly watchtower's
-- sight and away again: the side's picture shows it as seen, then as old with a growing age.
-- MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "an enemy seen, then lost",
  max_seconds = 120,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local C = require("rts.companies")
    local P = require("rts.picture")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Varric", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    B.place(1, "crown.watchtower", 0, -60, true)
    local scouts = S.new_company(2, "Riders")
    local rider = S.spawn_unit(2, "crown.outrider", 0, 60, scouts)
    local function sighting()
      for _, s in ipairs(P.list(1, prts.time.tick())) do
        if s.e == rider then return s end
      end
      return nil
    end
    t.ticks(30)
    t.expect(sighting(), "not seen at first").equals(nil)
    C.order(scouts, { kind = "move", x = 0, z = -20 })
    t.wait_until(function() local s = sighting() return s and s.state == "seen" end, 30)
    C.order(scouts, { kind = "move", x = 0, z = 200 })
    t.wait_until(function() local s = sighting() return s and s.state == "old" end, 40)
    local age1 = sighting().age
    t.ticks(150)
    local s = sighting()
    t.expect(s.state, "still old").equals("old")
    t.expect(s.age > age1 + 4, "and older (" .. s.age .. " s)").is_true()
    -- nothing reveals what no one has seen: the enemy holdfast is not in the picture
    local holds = 0
    for _, x in ipairs(P.list(1, prts.time.tick())) do
      if x.what == "structure" then holds = holds + 1 end
    end
    t.expect(holds, "no enemy building seen").equals(0)
  end,
}
