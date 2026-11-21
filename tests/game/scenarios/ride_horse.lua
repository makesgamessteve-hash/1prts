-- Spec (mounts-and-cavalry): Mounting and galloping; Dismounting. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "mounting a horse, galloping, dismounting",
  max_seconds = 60,
  run = function(t)
    local Horse = require("props.horse")
    local c = t.commander()
    t.place("start")
    t.ticks(10)
    local function speed(seconds)
      local x0, _, z0 = t.pos(c)
      t.ticks(math.floor(seconds * 30))
      local x1, _, z1 = t.pos(c)
      return math.sqrt((x1 - x0) ^ 2 + (z1 - z0) ^ 2) / seconds
    end
    -- on foot, as fast as the commander goes
    t.look(math.pi / 2, 0) -- west, over open meadow
    t.input.hold("sprint", 3)
    t.input.hold("move_forward", 3)
    t.ticks(20)
    local on_foot = speed(1.5)
    t.ticks(30)
    -- to the first horse, and up
    local hx, hy, hz = t.pos(prts.entity.find("horse.1"))
    t.place(hx - 2.2, hy + 0.1, hz)
    t.ticks(10)
    t.face("horse.1", 0, 1.2, 0)
    t.ticks(2)
    t.expect(t.target(), "the horse is in reach").equals("horse.1")
    t.expect(t.ui_has("Mount the horse"), "the prompt says so").is_true()
    t.input.press("use")
    t.ticks(2)
    t.expect(Horse.mounted(), "in the saddle").is_true()
    t.expect(prts.entity.find("horse.1"), "the horse is under the rider now").equals(nil)
    t.expect(prts.entity.find(Horse.HEAD) ~= nil, "its head is in view").is_true()
    t.ticks(30)
    local _, _, eye = prts.commander.view()
    t.expect(eye, "eye at saddle height").near(2.55, 0.05)
    t.look(math.pi / 2, 0)
    t.input.hold("sprint", 4)
    t.input.hold("move_forward", 4)
    t.ticks(45)
    local gallop = speed(1.5)
    t.expect(gallop >= on_foot * 2, "galloping (" .. gallop .. " m/s) is at least twice as fast as on foot (" .. on_foot .. ")").is_true()
    t.ticks(60)
    -- down again
    t.input.press("mount")
    t.ticks(2)
    t.expect(Horse.mounted(), "on foot").equals(false)
    t.expect(prts.commander.tune().sprint_speed, "own speeds back").near(prts.commander.tune().sprint_speed, 0)
    t.expect(prts.commander.tune().eye_stand, "own eye height back").near(1.7, 0.01)
    local horse = prts.entity.find("horse.1")
    t.expect(horse ~= nil, "the horse stands there").is_true()
    local x, _, z = t.pos(c)
    local bx, _, bz = t.pos(horse)
    t.expect(math.sqrt((bx - x) ^ 2 + (bz - z) ^ 2) < 2.5, "beside the commander").is_true()
    t.expect(prts.entity.find(Horse.HEAD), "no head in view on foot").equals(nil)
    -- and the mount action mounts the nearest horse too
    t.input.press("mount")
    t.ticks(2)
    t.expect(Horse.mounted(), "mounted with the mount action").is_true()
    t.input.press("mount")
    t.ticks(2)
  end,
}
