-- Harrowmark: the Raven Tower. Through its door and up the spiral stair, 36 m to the roof.
-- MIT License.
return {
  name = "the Raven Tower's spiral stair climbs to its roof",
  final_cell = "outside",
  max_seconds = 240,
  run = function(t)
    local map = require("maps.proving_ground")
    local R, c = map.RAVEN, t.commander()
    local cx, cz, radius = R.x, R.z, 2.6
    t.place("raven_tower_door")
    t.look(0, 0) -- north, into the tower
    t.input.hold("move_forward", 8)
    t.wait_until(function() local _, _, z = t.pos(c) return z < cz + 4.6 end, 8)
    -- walk on to the stair's circle, then follow it round and up
    t.input.hold("move_forward", 200)
    local top = false
    for _ = 1, 200 * 30 do
      local x, y, z = t.pos(c)
      if y >= R.height - 0.05 then top = true break end
      local a = math.atan(z - cz, x - cx)
      local r = math.sqrt((x - cx) ^ 2 + (z - cz) ^ 2)
      local pull = (r - radius) * 2
      local dx, dz = -math.sin(a) - math.cos(a) * pull, math.cos(a) - math.sin(a) * pull
      t.look(math.atan(-dx, -dz), 0)
      t.ticks(1)
    end
    t.input.release("move_forward")
    t.expect(top, "reached the top of the Raven Tower").is_true()
    -- step off the stair onto the roof
    t.look(math.pi, 0)
    t.input.hold("move_forward", 1.2)
    t.ticks(40)
    local x, y, z = t.pos(c)
    t.expect(y, "standing on the roof").near(R.height, 0.05)
    t.expect(math.sqrt((x - cx) ^ 2 + (z - cz) ^ 2) > radius + 0.8, "off the stair").is_true()
  end,
}
