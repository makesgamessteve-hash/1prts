-- Spec (vehicles-and-posts): Standing on a wagon; Driving the wagon. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "riding and driving the wagon",
  max_seconds = 120,
  run = function(t)
    local Deck = require("lib.deckplan")
    local Posts = require("lib.posts")
    local c = t.commander()
    local wagon = prts.entity.find("wagon")
    t.ticks(5)
    -- onto the bed, behind the bench
    local x, y, z = Deck.to_world(wagon, 0, 1.1, 1.0)
    t.place(x, y, z)
    t.ticks(5)
    t.expect(t.parent(c), "on the wagon's frame").equals("wagon")
    local yaw0 = prts.entity.yaw(wagon)
    local view0 = prts.commander.view()
    -- along the road until the wagon has turned the first corner
    t.wait_until(function()
      local d = (prts.entity.yaw(wagon) - yaw0 + math.pi) % (2 * math.pi) - math.pi
      return math.abs(d) > 1.2
    end, 60)
    t.expect(t.parent(c), "still riding").equals("wagon")
    local turned = (prts.entity.yaw(wagon) - yaw0 + math.pi) % (2 * math.pi) - math.pi
    local view_turned = (prts.commander.view() - view0 + math.pi) % (2 * math.pi) - math.pi
    t.expect(view_turned, "the view turned with the wagon").near(turned, 0.05)
    local wx, _, wz = t.pos(wagon)
    local cx, _, cz = t.pos(c)
    t.expect(math.sqrt((wx - cx) ^ 2 + (wz - cz) ^ 2) < 2.5, "carried along").is_true()
    -- the driver's seat
    t.face("wagon.bench")
    t.ticks(2)
    t.expect(t.target(), "the bench is in reach").equals("wagon.bench")
    t.input.press("use")
    t.ticks(2)
    t.expect(Posts.held() ~= nil, "on the driver's seat").is_true()
    t.expect(prts.entity.data(wagon).driven, "the wagon is driven").is_true()
    local x0, _, z0 = t.pos(wagon)
    t.input.hold("move_forward", 4)
    t.ticks(4 * 30)
    local x1, _, z1 = t.pos(wagon)
    t.expect(math.sqrt((x1 - x0) ^ 2 + (z1 - z0) ^ 2) > 6, "driven forward").is_true()
    t.expect(t.parent(c), "the driver rides along").equals("wagon")
    t.ticks(30)
    -- jump to leave the seat; the move keys move the commander again
    t.input.press("jump")
    t.ticks(30)
    t.expect(Posts.held(), "left the seat").equals(nil)
    t.expect(prts.entity.data(wagon).driven, "the wagon goes its own way again").equals(false)
    t.expect(prts.commander.tune().walk_speed > 1, "walking again").is_true()
  end,
}
