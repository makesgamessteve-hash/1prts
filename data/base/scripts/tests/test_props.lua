-- Lua unit tests: doors and the hoist lift (logic only; motion is simulated by the engine). MIT License.
local Door = require("props.door")
local Lift = require("props.lift")
local map = require("maps.proving_ground")

T.case("the map has its markers and zones", function()
  for _, name in ipairs({ "start", "keep_door", "keep_hall_lift", "stair_base", "war_room_view" }) do
    T.is_true(prts.scene.marker_pos(name) ~= nil, name)
  end
  T.is_true(map.trees > 50, "trees")
  T.is_true(prts.entity.find("keep.door") ~= nil, "door entity")
end)

T.case("doors toggle open and closed", function()
  local d = prts.entity.find("keep.forge.door")
  T.eq(prts.entity.data(d).open, false)
  T.is_true(Door.on_use(d))
  T.eq(prts.entity.data(d).open, true)
  T.is_true(prts.entity.moving(d))
  T.eq(Door.toggle(d), false)
  T.eq(Door.on_use(map.lift), false, "a lift is not a door")
end)

T.case("lift call levers and the platform lever pick floors", function()
  local lift = map.lift
  local state = prts.entity.data(lift)
  T.eq(state.current, 1)
  -- calling the lift to the floor it is on just opens that gate
  T.eq(Lift.on_use(state.calls[1]), false)
  T.eq(prts.entity.data(state.gates[1]).open, true)
  -- the platform lever sends it to the top floor and closes every gate
  T.eq(Lift.on_use(state.lever), true)
  T.eq(state.target, 3)
  T.eq(prts.entity.data(state.gates[1]).open, false)
  T.is_true(prts.entity.moving(lift))
  -- while moving, requests are ignored
  T.eq(Lift.go_to(lift, 2), false)
  -- arriving opens the gate of the new floor
  T.eq(Lift.on_move_done(lift), true)
  T.eq(state.current, 3)
  T.eq(prts.entity.data(state.gates[3]).open, true)
  T.eq(Lift.set_gate(lift, 3, true), false, "already open")
  T.eq(Lift.on_move_done(state.gates[3]), false, "gates are not lifts")
  T.eq(Lift.on_use(prts.entity.find("keep.door")), false, "a door is not a lift lever")
end)

T.case("the platform lever goes up, or down from the top", function()
  T.eq(Lift.lever_target({ floors = { 0, 4, 8 }, current = 1 }), 3)
  T.eq(Lift.lever_target({ floors = { 0, 4, 8 }, current = 2 }), 3)
  T.eq(Lift.lever_target({ floors = { 0, 4, 8 }, current = 3 }), 1)
end)
