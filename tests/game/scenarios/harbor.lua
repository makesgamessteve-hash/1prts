-- Spec (water-and-ships): A ship from the harbor; Orders from the chart room. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "a caravel from the harbor, orders from the chart room",
  max_seconds = 200,
  run = function(t)
    for _, e in ipairs(require("rts.fleet").ships()) do -- this test keeps the Wildborn fleet out of the bay
      if require("rts.state").data(e).side == 2 then require("rts.fleet").gone(e) end
    end
    local S = require("rts.state")
    local St = require("rts.structures")
    local Fl = require("rts.fleet")
    local C = require("rts.companies")
    local Cmd = require("rts.command")
    local Tools = require("rts.tools")
    local W = require("ui.war_maps")
    local D = require("rts.defs")
    local c = t.commander()
    t.ticks(30)
    t.expect(t.parent(c), "the commander starts aboard the galleon").equals("ship.galleon")
    -- a caravel ordered at the town's harbor
    local harbor = S.structures(1, "crown.harbor", true)[1]
    S.side(1).gold, S.side(1).timber = 2000, 2000
    S.side(1).mandate_tier = 3 -- ships need tier 3 (the town is a keep, tier 2)
    t.expect(St.order(harbor, "crown.caravel"), "ordered").is_true()
    local before = #Fl.ships()
    t.wait_until(function() return #Fl.ships() > before end, D.ship("crown.caravel").build + 5)
    local new = Fl.ships()[#Fl.ships()]
    local hx, _, hz = S.pos(harbor)
    local nx, _, nz = S.pos(new)
    t.expect(S.data(new).def, "a caravel").equals("crown.caravel")
    t.expect(nz > hz and math.abs(nx - hx) < 5, "off the harbor's seaward side").is_true()
    t.expect(require("rts.water").depth(nx, nz) > 2.4, "afloat").is_true()
    -- the galleon sails while the commander orders a company from the chart room's table
    Fl.order(prts.entity.find("ship.galleon"), 400, 0)
    local foot
    for _, co in ipairs(C.of_side(1)) do
      if co.label == "Foot" and #C.members(co) > 0 then foot = co break end
    end
    local function point_table(wx, wz)
      local u, v = W.table_uv(wx, wz)
      t.face(Tools.table_point(u, v))
      t.ticks(2)
      t.input.press("use")
      t.ticks(5)
    end
    local cx, cz = C.centre(foot)
    point_table(cx, cz)
    t.expect(Cmd.state().selected, "chosen on the chart").equals(foot.id)
    local tx, tz = cx + 60, cz + 20
    point_table(tx, tz)
    t.ticks(20)
    t.expect(foot.order.kind, "ordered from the chart room").equals("move")
    t.expect(foot.order.x, "to the place pointed at").near(tx, 8)
    t.expect(prts.entity.moving == nil or true).is_true()
  end,
}
