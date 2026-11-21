-- Spec (commander-locomotion): Reaching the war room. The commander crosses the war room to the
-- war table, is stopped by its edge (the table is solid, too high to step onto) and looks down
-- onto the live map from there. Part of the commander-is-there smoke set (task 10.5), which
-- also runs on the web build (web/tests/smoke.test.js). MIT License.
-- ctest-labels: smoke
return {
  name = "the commander walks up to the war table and looks down onto it",
  final_cell = "keep.floor3",
  run = function(t)
    local map = require("maps.proving_ground")
    local c = t.commander()
    local floor = map.FLOORS[3]
    t.place("war_room_view") -- the south end of the war room, facing north towards the table
    t.ticks(2)
    t.expect(t.in_zone(c, "keep.floor3.war_room"), "in the war room").is_true()
    local x0, _, z0 = t.pos(c)
    t.input.hold("move_forward", 4)
    t.ticks(120)
    local x, y, z = t.pos(c)
    -- the table top spans z 1.2 to 3.8: the commander stands at its south edge, not on it
    t.expect(z0 - z > 2.5, "walked towards the table").is_true()
    t.expect(z > 3.8 and z < 4.3, "stopped at the table's edge (z " .. z .. ")").is_true()
    t.expect(x, "kept to the straight line").near(x0, 0.05)
    t.expect(y, "still on the war room floor").near(floor, 0.05)
    -- from there the table's centre is in reach of a look down
    t.face(0, floor + 1.0, 2.5)
    t.ticks(1)
    local _, pitch = prts.commander.view()
    t.expect(pitch < -0.3 and pitch > -1.2, "looking down onto the table (pitch " .. pitch .. ")").is_true()
    t.expect(prts.scene.cell_at(0, floor + 1.0, 2.5), "the table stands in the war room").equals("keep.floor3")
  end,
}
