-- Lua unit tests: trigger zones. MIT License.
local Tr = require("lib.triggers")

T.case("watched entities send zone_enter and zone_exit", function()
  prts.scene.zone("test.room", { 100, 0, 100 }, { 110, 5, 110 })
  local inside = prts.entity.spawn{ name = "test.in", pos = { 105, 0, 105 } }
  local outside = prts.entity.spawn{ name = "test.out", pos = { 0, 0, 0 } }
  local got = {}
  prts.events.on("zone_enter", "test", function(zone, e) got[#got + 1] = "enter " .. zone .. " " .. prts.entity.name(e) end)
  prts.events.on("zone_exit", "test", function(zone) got[#got + 1] = "exit " .. zone end)
  local a = Tr.watch("test.room", inside)
  local b = Tr.watch("test.room", outside)
  T.eq(Tr.check(), 1)
  T.eq(got[1], "enter test.room test.in")
  T.eq(Tr.check(), 0, "no change, no events")
  prts.entity.despawn(inside)
  T.eq(Tr.check(), 1, "a removed entity has left")
  T.eq(got[2], "exit test.room")
  T.eq(Tr.unwatch(a), true)
  T.eq(Tr.unwatch(a), false)
  T.eq(Tr.unwatch(b), true)
  T.fails(function() prts.scene.in_zone("no.such.zone", 0, 0, 0) end)
  T.eq(prts.scene.in_zone("test.room", 105, 1, 105), true)
  T.eq(prts.scene.in_zone("test.room", 105, 9, 105), false)
  prts.events.off("zone_enter", "test")
  prts.events.off("zone_exit", "test")
end)
