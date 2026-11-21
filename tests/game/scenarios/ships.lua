-- Spec (water-and-ships): A cog lands troops. A company comes aboard a cog off the town's shore,
-- the cog sails across the bay, and the company is landed on the far side. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "a cog lands troops across the bay",
  max_seconds = 300,
  run = function(t)
    for _, e in ipairs(require("rts.fleet").ships()) do -- this test keeps the Wildborn fleet out of the bay
      if require("rts.state").data(e).side == 2 then require("rts.fleet").gone(e) end
    end
    local S = require("rts.state")
    local C = require("rts.companies")
    local Fl = require("rts.fleet")
    local Wa = require("rts.water")
    local Cmd = require("rts.command")
    local map = require("maps.saltcoast")
    -- deep water off the town's shore, and the shore beside it
    local x = map.TOWN.x - 70
    local deep, shore
    for z = map.TOWN.z, 0, 2 do
      if not shore and Wa.depth(x, z) > 0 then shore = z - 6 end
      if Wa.depth(x, z) >= 3.2 then deep = z break end
    end
    local cog = Fl.spawn(1, "crown.cog", x, deep + 4, 0, "ship.test_cog")
    local co = S.new_company(1, "Foot")
    for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", x - 6 + (i % 4) * 1.4, shore - 4 - i // 4 * 1.4, co) end
    t.ticks(30)
    local aboard = Fl.embark(cog, co)
    t.expect(aboard, "all eight come aboard").equals(8)
    t.expect(#C.members(co), "off the field").equals(0)
    t.expect(co.aboard, "aboard the cog").equals("ship.test_cog")
    -- across the bay: pointing at the far shore from the war table sails there and lands them
    Cmd.state().ship = "ship.test_cog"
    local land
    for z = 0, map.CAMP.z, 2 do
      if Wa.depth(x, z) == 0 then land = z + 10 break end
    end
    t.expect(Cmd.point(x, land), "the cog is sent to land them").equals("landing")
    t.wait_until(function() return #C.members(co) == 8 end, 240)
    for _, e in ipairs(C.members(co)) do
      local ex, _, ez = S.pos(e)
      t.expect(ez > 100, "ashore on the far side").is_true()
      t.expect(Wa.depth(ex, ez), "on dry ground").equals(0)
    end
    t.expect(co.aboard, "no longer aboard").equals(nil)
    t.expect(#prts.entity.data(cog).cargo, "the hold is empty").equals(0)
  end,
}
