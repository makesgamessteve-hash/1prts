-- Spec (deck-plans-and-crews): The tower's push room is wrecked. Its people fall and the push
-- bar no longer moves the tower. Also: the fort's towers shoot at the siege tower on its way.
-- MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "the siege tower's push room is wrecked",
  max_seconds = 30,
  run = function(t)
    local Crew = require("lib.crew")
    local Tower = require("props.siege_tower")
    local Posts = require("lib.posts")
    local map = require("maps.greyfen_march")
    local tower = prts.entity.find("siege_tower")
    local d = prts.entity.data(tower)
    local wrecked = {}
    prts.events.on("room_wrecked", "test.rooms", function(_, room) wrecked[#wrecked + 1] = room end)
    t.place(map.TOWER.x - 12, prts.scene.height(map.TOWER.x - 12, 0) + 0.1, 0)
    t.ticks(20)
    -- the fort shoots: a bolt every two seconds into one of the rooms
    local before = d.rooms.push.hp + d.rooms.middle.hp + d.rooms.top.hp
    t.ticks(130)
    t.expect(d.rooms.push.hp + d.rooms.middle.hp + d.rooms.top.hp < before, "bolts struck the tower").is_true()
    t.expect(Crew.figures(tower), "embodied").equals(10)
    Tower.damage(tower, "push", 80)
    t.expect(#Crew.living(tower, "push"), "a quarter gone costs one pusher").equals(7)
    t.expect(Tower.push_speed(tower) < Tower.SPEED, "fewer pushers push slower").is_true()
    Tower.damage(tower, "push", 500)
    t.expect(d.rooms.push.wrecked, "wrecked").is_true()
    t.expect(wrecked[1], "the event names the room").equals("push")
    t.expect(#Crew.living(tower, "push"), "four of the eight fell").equals(4)
    t.expect(Crew.figures(tower), "their figures are gone").equals(6)
    t.expect(Tower.push_speed(tower), "nothing moves the tower").equals(0)
    t.expect(Posts.take(prts.entity.find(d.post)), "the push bar is broken").equals(false)
    t.expect(Tower.damage(tower, "push", 10).wrecked, "a wrecked room stays wrecked").is_true()
    t.expect(Tower.lower_bridge(tower), "no bridge before the wall").equals(false)
  end,
}
