-- Spec (command-tools): Ordering a company from the war table. The commander, in the war room of
-- the holdfast, points at a company on the table (it is chosen) and then at a place on the table:
-- the company marches to the matching place in the world. Training at the table works the same
-- way. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "ordering a company from the war table",
  max_seconds = 60,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local Cmd = require("rts.command")
    local Tools = require("rts.tools")
    local W = require("ui.war_maps")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 2 }
    local foot = S.new_company(1, "Foot")
    for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", V.WEST.x + 40 + (i % 4) * 1.4, V.WEST.z + 40 + i // 4 * 1.4, foot) end
    require("rts.structures").place(1, "crown.farmstead", V.WEST.x - 40, V.WEST.z + 40, true) -- food for a serf
    t.place("start")
    t.ticks(30) -- settle on the floor
    t.expect(t.ui_has("Gold", "war_table"), "the table shows the stock").is_true()
    local function point_table(wx, wz)
      local u, v = W.table_uv(wx, wz)
      t.face(Tools.table_point(u, v))
      t.ticks(2)
      t.input.press("use")
      t.ticks(5) -- the press, then a frame drawn with its result
    end
    local cx, cz = require("rts.companies").centre(foot)
    point_table(cx, cz)
    t.expect(Cmd.state().selected, "the company is chosen").equals(foot.id)
    t.expect(t.ui_has(foot.name, "war_table"), "the table names it").is_true()
    local tx, tz = V.WEST.x + 120, V.WEST.z - 60
    point_table(tx, tz)
    t.ticks(20)
    t.expect(foot.order.kind, "ordered to march").equals("move")
    t.expect(foot.order.x, "to the place pointed at (x)").near(tx, 4)
    t.expect(foot.order.z, "to the place pointed at (z)").near(tz, 4)
    t.expect(prts.agent.info(foot.members[1]).moving, "marching").is_true()
    -- training: the "Serf" button on the table orders a serf at the holdfast
    local u, v = W.hot_uv("war_table", "train:crown.serf")
    local gold = S.side(1).gold
    t.face(Tools.table_point(u, v))
    t.ticks(2)
    t.input.press("use")
    t.ticks(5)
    t.expect(S.side(1).gold, "a serf is paid for").equals(gold - 50)
  end,
}
