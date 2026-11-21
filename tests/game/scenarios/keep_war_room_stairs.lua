-- Spec (commander-locomotion): Stairs. The keep's switchback stairs all the way from the great
-- hall to the war room, standing up (the floor over the upper landing used to leave too little
-- headroom: only a crouching commander got through; the fun audit's bots found it). MIT License.
return {
  name = "the keep's stairs climb from the great hall to the war room",
  final_cell = "keep.floor3",
  max_seconds = 60,
  run = function(t)
    local c = t.commander()
    t.place("stair_base")
    t.ticks(2)
    local function walk_to(x, z)
      t.wait_until(function()
        local px, _, pz = t.pos(c)
        local dx, dz = x - px, z - pz
        if dx * dx + dz * dz < 0.2 then return true end
        t.look(math.atan(-dx, -dz), 0)
        t.input.hold("move_forward", 0.1)
        return false
      end, 10)
    end
    for _, p in ipairs({ { 9.8, -7.4 }, { 7.2, -7.4 }, { 7.2, -2.4 }, { 9.8, -2.4 }, { 9.8, -7.4 }, { 7.2, -7.4 }, { 7.2, -2.4 },
                         { 7.2, 0.0 }, { 3.0, 0.0 } }) do -- and out of the stairwell into the room
      walk_to(p[1], p[2])
    end
    local _, y = t.pos(c)
    t.expect(y, "war room floor").near(9.2, 0.05)
    t.expect(t.in_zone(c, "keep.floor3.war_room"), "in the war room").is_true()
    local _, _, _, ground = prts.commander.view()
    t.expect(ground, "standing on it").is_true()
  end,
}
