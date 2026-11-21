-- Spec (command-tools): Advance to my mark; The rally horn. The commander marks a point on the
-- ground with the staff and chooses "advance to my mark" on the sending stone: the company moves
-- to the mark. The rally signal on the horn brings every company within 2 km to the commander.
-- MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "advance to my mark, and the rally horn",
  max_seconds = 120,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local C = require("rts.companies")
    local Cmd = require("rts.command")
    local Tools = require("rts.tools")
    local W = require("ui.war_maps")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local foot, bows = S.new_company(1, "Foot"), S.new_company(1, "Bows")
    for i = 1, 8 do
      S.spawn_unit(1, "crown.men_at_arms", V.WEST.x + 30 + (i % 4) * 1.4, V.WEST.z + 50, foot)
      S.spawn_unit(1, "crown.crossbowmen", V.WEST.x - 30 + (i % 4) * 1.4, V.WEST.z + 50, bows)
    end
    t.place("gate")
    t.ticks(2)
    -- the staff: mark a point 25 m ahead on the ground
    local mx, mz = V.WEST.x + 5, V.WEST.z + 45
    t.face(mx, prts.scene.height(mx, mz), mz)
    t.ticks(1)
    t.input.press("mark")
    t.ticks(2)
    local mark = Cmd.state().mark
    t.expect(mark ~= nil, "marked").is_true()
    t.expect(mark[1], "where the commander looked (x)").near(mx, 1.5)
    t.expect(mark[2], "where the commander looked (z)").near(mz, 1.5)
    t.expect(prts.entity.find("rts.mark") ~= nil, "the staff stands there").is_true()
    -- the stone: choose the foot, then "advance to my mark"
    Cmd.select(foot.id)
    t.input.press("stone")
    t.ticks(3)
    t.expect(Tools.held(), "the stone is up").equals("stone")
    local function choose(id)
      local u, v = W.hot_uv("rts.hand", id)
      t.expect(u ~= nil, "the board offers " .. id).is_true()
      t.face(Tools.hand_point(u, v))
      t.ticks(2)
      t.input.press("use")
      t.ticks(2)
    end
    choose("advance")
    t.expect(Tools.held(), "the stone is put away").equals(nil)
    t.ticks(20)
    t.expect(foot.order.kind, "the foot marches").equals("move")
    t.wait_until(function()
      local cx, cz = C.centre(foot)
      return S.dist(cx, cz, mark[1], mark[2]) < 4
    end, 30)
    -- the horn: rally every company to the commander
    local hx, _, hz = t.pos(t.commander())
    t.input.press("horn")
    t.ticks(3)
    t.expect(Tools.held(), "the horn is up").equals("horn")
    choose("rally")
    t.expect(foot.order.kind, "the foot comes").equals("move")
    t.expect(bows.order.kind, "the bows come").equals("move")
    t.expect(bows.order.x, "to the commander").near(hx, 1)
    t.wait_until(function()
      local cx, cz = C.centre(bows)
      return S.dist(cx, cz, hx, hz) < 6
    end, 40)
  end,
}
