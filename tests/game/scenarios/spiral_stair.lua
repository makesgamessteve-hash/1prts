-- Spec (commander-locomotion): Stairs. From the war room floor the commander walks into the
-- keep's spiral stair and keeps moving forward (turning with it) until they come out on the
-- roof through the hatch. MIT License.
-- ctest-labels: smoke
return {
  name = "the spiral stair leads from the war room floor to the roof",
  final_cell = "outside",
  run = function(t)
    local map = require("maps.proving_ground")
    local sp, c = map.SPIRAL, t.commander()
    t.place("spiral_base")
    t.ticks(2)
    t.expect(t.in_cell(c, "keep.stairs"), "at the foot of the stair").is_true()
    t.input.hold("move_forward", 20)
    local top = false
    for _ = 1, 20 * 30 do
      local x, y, z = t.pos(c)
      if y >= map.ROOF - 0.05 then top = true break end
      -- keep facing along the turn, pulled towards the walking radius
      local a = math.atan(z - sp.z, x - sp.x)
      local r = math.sqrt((x - sp.x) ^ 2 + (z - sp.z) ^ 2)
      local pull = (r - sp.radius) * 2
      local dx, dz = -math.sin(a) - math.cos(a) * pull, math.cos(a) - math.sin(a) * pull
      t.look(math.atan(-dx, -dz), 0)
      t.ticks(1)
    end
    t.input.release("move_forward")
    t.expect(top, "reached the roof's height by walking").is_true()
    -- step off the stair onto the roof, towards the south
    t.look(math.pi, 0)
    t.input.hold("move_forward", 1.5)
    t.ticks(45)
    local _, y, z = t.pos(c)
    t.expect(z > map.HATCH.z1, "past the hatch").is_true()
    t.expect(y, "standing on the roof").near(map.ROOF, 0.05)
    t.expect(t.in_cell(c, "outside"), "out on the roof").is_true()
  end,
}
