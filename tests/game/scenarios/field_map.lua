-- Spec (command-tools): Opening the field map. The commander opens the field map while running:
-- the map shows the side's companies and the commander slows to walking. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "opening the field map",
  max_seconds = 60,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local Tools = require("rts.tools")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 2 }
    local bows = S.new_company(1, "Bows")
    for i = 1, 8 do S.spawn_unit(1, "crown.crossbowmen", V.WEST.x + 40 + (i % 4) * 1.4, V.WEST.z + 60, bows) end
    t.place("gate")
    t.look(0, 0)
    t.ticks(2)
    t.input.press("next_company")
    t.ticks(2)
    local c = t.commander()
    local function speed(seconds)
      local x0, _, z0 = t.pos(c)
      t.ticks(math.floor(seconds * 30))
      local x1, _, z1 = t.pos(c)
      return math.sqrt((x1 - x0) ^ 2 + (z1 - z0) ^ 2) / seconds
    end
    t.input.hold("run", 6)
    t.input.hold("move_forward", 6)
    t.ticks(15)
    local running = speed(1)
    t.input.press("field_map")
    t.ticks(10)
    local walking = speed(1)
    t.expect(Tools.held(), "the map is up").equals("map")
    t.expect(running > 3, "ran first (" .. running .. " m/s)").is_true()
    t.expect(walking < 1.6, "walks with the map up (" .. walking .. " m/s)").is_true()
    t.expect(t.ui_has(bows.name, "rts.hand"), "the map shows the chosen company").is_true()
    t.input.release("run")
    t.input.release("move_forward")
    t.input.press("field_map")
    t.ticks(2)
    t.expect(Tools.held(), "put away").equals(nil)
    t.input.hold("run", 2)
    t.input.hold("move_forward", 2)
    t.ticks(10)
    t.expect(speed(1) > 3, "runs again").is_true()
  end,
}
