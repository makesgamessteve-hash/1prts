-- Spec (vehicles-and-posts): Walking the galleries of a gold mine. The commander walks through the
-- adit into the lower gallery, rides the lift cage up and steps out into the upper gallery.
-- MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "walking the galleries of the gold mine",
  max_seconds = 90,
  run = function(t)
    local Mine = require("props.mine")
    local c = t.commander()
    local mine = prts.entity.find("gold_mine")
    local cage = Mine.cage(mine)
    t.place("adit")
    t.look(0, 0) -- north, into the hill
    t.ticks(10)
    t.input.hold("move_forward", 5)
    t.wait_until(function() return t.in_zone(c, "gold_mine.lower gallery") end, 6)
    t.ticks(5 * 30)
    -- along the gallery to the cage
    local px, py, pz = t.pos(cage)
    t.place(px - 2.0, py + 0.05, pz, -math.pi / 2, 0)
    t.ticks(10)
    t.expect(t.in_zone(c, "gold_mine.lower gallery"), "at the cage's gate").is_true()
    t.input.hold("move_forward", 1.2)
    t.ticks(40)
    t.expect(t.parent(c), "standing in the cage").equals("gold_mine.cage.platform")
    t.face("gold_mine.cage.lever")
    t.ticks(2)
    t.expect(t.target(), "the cage's lever").equals("gold_mine.cage.lever")
    t.input.press("use")
    t.wait_until(function()
      local _, y = t.pos(c)
      return y > py + 7.9 and not prts.entity.moving(cage)
    end, 15)
    t.ticks(60) -- the gate opens
    t.look(math.pi / 2, 0)
    t.input.hold("move_forward", 2)
    t.wait_until(function() return t.in_zone(c, "gold_mine.upper gallery") end, 4)
  end,
}
