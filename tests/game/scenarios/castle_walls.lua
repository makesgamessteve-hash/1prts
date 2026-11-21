-- Harrowmark: from the Inner Ward up the stair to the wall-walk, along the battlements and
-- into the south-west tower at wall-walk height. MIT License.
return {
  name = "the wall stair leads onto the inner wall-walk and into a tower",
  final_cell = "outside",
  run = function(t)
    local map = require("maps.proving_ground")
    local I, c = map.INNER, t.commander()
    t.place("inner_wall_stair")
    t.ticks(2)
    t.input.hold("run", 14)
    t.input.hold("move_forward", 14)
    t.wait_until(function() local _, y = t.pos(c) return y > I.height - 0.05 end, 14)
    t.input.release("move_forward")
    t.ticks(2)
    -- step onto the wall-walk (south) and walk west along it
    t.look(math.pi, 0)
    t.input.hold("move_forward", 0.8)
    t.ticks(30)
    local _, y, z = t.pos(c)
    t.expect(y, "on the wall-walk").near(I.height, 0.05)
    t.expect(z > I.z1 and z < I.z1 + I.thick, "between the faces of the wall").is_true()
    t.look(math.pi / 2, 0) -- west
    t.input.hold("move_forward", 6)
    t.ticks(180)
    local x
    x, y, z = t.pos(c)
    local tx, tz = I.x0 - I.thick / 2, I.z1 + I.thick / 2 -- the south-west tower's centre
    t.expect(math.sqrt((x - tx) ^ 2 + (z - tz) ^ 2) < 4.5, "inside the corner tower").is_true()
    t.expect(y, "on its upper floor").near(I.height, 0.05)
  end,
}
