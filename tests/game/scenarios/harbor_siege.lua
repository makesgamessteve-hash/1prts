-- Spec (harbor-siege): Breaking the siege; The galleon lost. MIT License.
-- ctest-labels: slow
-- ctest-args: --start saltcoast
return {
  name = "the Harbor Siege",
  max_seconds = 1500,
  run = function(t)
    local S = require("rts.state")
    local C = require("rts.companies")
    local Cmd = require("rts.command")
    local match = require("rts.match")
    local map = require("maps.saltcoast")
    local c = t.commander()
    t.ticks(30)
    t.expect(t.parent(c), "the commander in the galleon").equals("ship.galleon")
    t.expect(t.ui_has("Gold", "war_table"), "at the chart room's table").is_true()
    -- the galleon leads the fleet into the bay; the cogs land the relief by the siege camp
    local Fl = require("rts.fleet")
    -- the galleon comes in as close to the camp as her draft allows, to bombard it
    local Wa0 = require("rts.water")
    local gz = 0
    for z = 0, map.CAMP.z, 2 do
      if Wa0.depth(map.CAMP.x, z) < 4.5 then break end
      gz = z
    end
    Fl.order(prts.entity.find("ship.galleon"), map.CAMP.x, gz)
    Fl.order(prts.entity.find("ship.caravel"), map.CAMP.x + 100, -60)
    local Wa = require("rts.water")
    for _, name in ipairs({ "ship.cog.1", "ship.cog.2" }) do
      local x = map.CAMP.x - 40 + (name == "ship.cog.2" and 80 or 0)
      local land
      for z = 0, map.CAMP.z, 2 do
        if Wa.depth(x, z) == 0 then land = z + 8 break end
      end
      Cmd.state().ship = name
      t.expect(Cmd.point(x, land), name .. " sent to land its troops").equals("landing")
    end
    local landed = 0
    prts.events.on("rts_landed", "test.siege", function(_, n) landed = landed + n end)
    -- the cogs land what they carry (one may be lost to the longships on the way)
    t.wait_until(function()
      for _, e in ipairs(Fl.ships()) do
        if S.data(e).side == 1 and #S.data(e).cargo > 0 then return false end
      end
      return true
    end, 400)
    t.expect(landed >= 12, "the relief is ashore (" .. landed .. " men)").is_true()
    -- every company ashore takes the camp's war lodge
    local lodge = S.hq(2)
    local lx, _, lz = S.pos(lodge)
    for _, co in ipairs(C.of_side(1)) do
      if #C.members(co) > 0 then C.order(co, { kind = "attack", x = lx, z = lz }) end
    end
    for _ = 1, 40 do
      t.ticks(30 * 15)
      if not match.running() then break end
      for _, co in ipairs(C.of_side(1)) do
        if #C.members(co) > 0 and co.order.kind ~= "attack" and not co.broken then C.order(co, { kind = "attack", x = lx, z = lz }) end
      end
    end
    t.expect(match.winner(), "the siege is broken").equals(1)
    prts.log.info(string.format("siege broken at minute %.1f", (S.match().ended - S.match().started) / 1800))
  end,
}
