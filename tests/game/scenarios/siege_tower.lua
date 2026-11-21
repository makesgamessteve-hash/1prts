-- Spec (vehicles-and-posts): Up the tower to the wall. The commander sets the crew pushing at the
-- push bar, climbs the stairs to the top floor while the tower rolls, pulls the bridge lever once
-- the tower is at the wall and walks across onto the wall-walk. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "up the siege tower to the wall",
  max_seconds = 120,
  run = function(t)
    local Deck = require("lib.deckplan")
    local Tower = require("props.siege_tower")
    local map = require("maps.greyfen_march")
    local c = t.commander()
    local tower = prts.entity.find("siege_tower")
    local d = prts.entity.data(tower)
    local function at(lx, ly, lz, yaw)
      local x, y, z = Deck.to_world(tower, lx, ly, lz)
      t.place(x, y, z, yaw or 0, 0)
    end
    -- inside on the ground floor, at the push bar
    at(0, Tower.LIFT + 0.05, 0, -math.pi / 2)
    t.ticks(10)
    t.expect(t.parent(c), "inside the tower's frame").equals("siege_tower")
    t.face(d.post)
    t.ticks(2)
    t.expect(t.target(), "the push bar").equals(d.post)
    t.input.press("use")
    t.ticks(5)
    t.expect(d.pushing, "the crew push").is_true()
    local x0 = t.pos(tower)
    -- up the first stair while the tower rolls (it climbs towards the front)
    at(1.2, Tower.LIFT + 0.05, 1.8, -math.pi / 2)
    t.ticks(5)
    local _, y_start = t.pos(c)
    t.input.hold("move_forward", 4)
    t.ticks(4 * 30)
    local _, y1 = t.pos(c)
    t.expect(y1 - y_start > 2.8, "up one floor (" .. (y1 - y_start) .. " m)").is_true()
    t.expect(t.parent(c), "still in the tower").equals("siege_tower")
    t.expect(t.pos(tower) > x0 + 1, "the tower rolled meanwhile").is_true()
    -- and the second stair (it climbs towards the back)
    at(-1.2, Tower.LIFT + 3.2 + 0.05, -1.8, math.pi / 2)
    t.ticks(5)
    t.input.hold("move_forward", 4)
    t.ticks(4 * 30)
    local _, y2 = t.pos(c)
    t.expect(y2 - y_start > 6.0, "on the top floor").is_true()
    -- at the wall, the bridge goes down
    t.wait_until(function() return d.at_wall end, 60)
    t.expect(d.pushing, "the crew stop at the wall").equals(false)
    t.expect(map.distance_to_wall(tower), "front at the wall").near(Tower.STOP, 0.05)
    at(0.4, Tower.LIFT + 6.45, -1.2, -math.pi / 2) -- across the top floor to the lever
    t.ticks(5)
    t.face(d.lever)
    t.ticks(2)
    t.expect(t.target(), "the bridge lever").equals(d.lever)
    t.input.press("use")
    t.ticks(4 * 30)
    t.expect(d.bridge_down, "the bridge is down").is_true()
    -- across onto the wall-walk
    at(0, Tower.LIFT + 6.45, -2.4, -math.pi / 2)
    t.ticks(5)
    t.input.hold("move_forward", 2.5) -- the wall-walk is 3 m wide: stop on it
    t.ticks(3 * 30)
    local x, y = t.pos(c)
    t.expect(x > map.FORT.x0 + 0.5, "on the wall (x " .. x .. ")").is_true()
    t.expect(x < map.FORT.x0 + map.FORT.thick, "still on it").is_true()
    t.expect(y, "at the wall-walk's height").near(map.fort_top, 0.2)
    -- the bridge lies on the wall-walk: a step to the side is off the tower's frame
    t.input.hold("move_left", 1.2)
    t.ticks(45)
    local _, y3 = t.pos(c)
    t.expect(t.parent(c), "off the tower's frame").equals("world")
    t.expect(y3, "on the wall-walk").near(map.fort_top, 0.05)
  end,
}
