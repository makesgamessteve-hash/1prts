-- Spec (mounts-and-cavalry): Ride with your knights. The mounted commander orders the knights
-- to ride with them and gallops away; the knights follow and are close behind when the
-- commander stops. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "ride with your knights",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local C = require("rts.companies")
    local Cmd = require("rts.command")
    local Horse = require("props.horse")
    local c = t.commander()
    local knights
    for _, co in ipairs(C.of_side(1)) do
      if co.label == "Knights" then knights = co end
    end
    t.expect(knights ~= nil, "the camp's knights").is_true()
    t.place("start")
    t.ticks(10)
    Horse.mount(prts.entity.find("horse.2"))
    Cmd.select(knights.id)
    t.expect(Cmd.order("follow"), "the order goes over the stones").is_true()
    t.ticks(20)
    t.expect(knights.order.kind, "riding with the commander").equals("follow")
    -- gallop 120 m east, then stop
    t.look(-math.pi / 2, 0)
    t.input.hold("sprint", 11)
    t.input.hold("move_forward", 11)
    t.ticks(11 * 30)
    t.ticks(30 * 20)
    local x, _, z = t.pos(c)
    local kx, kz = C.centre(knights)
    local dist = S.dist(x, z, kx, kz)
    t.expect(dist < 16, "the knights are close behind (" .. string.format("%.1f", dist) .. " m)").is_true()
    -- and the horn's "Ride with me" does the same for every company in earshot
    t.expect(Cmd.horn("follow") >= 1, "heard").is_true()
    t.expect(knights.order.kind).equals("follow")
  end,
}
