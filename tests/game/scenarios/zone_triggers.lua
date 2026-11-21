-- Trigger zones: walking into the great hall and out again sends zone_enter and zone_exit
-- events to scripts (lib/triggers.lua). MIT License.
return {
  name = "the great hall reports the commander entering and leaving",
  run = function(t)
    local Tr = require("lib.triggers")
    local events = {}
    prts.events.on("zone_enter", "scenario", function(zone) events[#events + 1] = "enter " .. zone end)
    prts.events.on("zone_exit", "scenario", function(zone) events[#events + 1] = "exit " .. zone end)
    local watch = Tr.watch("keep.great_hall")
    t.place("keep_door")
    t.ticks(2)
    t.expect(#events, "nothing yet outside").equals(0)
    t.place("hall_view")
    t.ticks(2)
    t.expect(events[1], "entered").equals("enter keep.great_hall")
    t.place("start")
    t.ticks(2)
    t.expect(events[2], "left").equals("exit keep.great_hall")
    Tr.unwatch(watch)
  end,
}
