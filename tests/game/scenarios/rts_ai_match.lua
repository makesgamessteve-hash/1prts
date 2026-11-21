-- Spec (rts-core): An opponent. The AI commander gathers, builds, trains and attacks on its own; a
-- match ends when a side's holdfast falls. The AI plays a side that does nothing (its serfs only
-- work): it must take that side's holdfast within 30 minutes of game time. (Two equal AI
-- commanders on this mirrored map often fight to a standstill for an hour.) MIT License.
-- ctest-labels: slow
-- ctest-args: --start ashford_vale
return {
  name = "the AI commander takes a holdfast",
  max_seconds = 2400,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local V = require("maps.ashford_vale")
    local events = {}
    for _, ev in ipairs({ "rts_structure_built", "rts_unit_spawned", "rts_ai_attack", "rts_match_over" }) do
      prts.events.on(ev, "test.ai." .. ev, function() events[ev] = (events[ev] or 0) + 1 end)
    end
    match.start{ sides = { { id = 1, name = "Crown", color = "crown", ai = false, x = V.WEST.x, z = V.WEST.z },
                           { id = 2, name = "Varric", color = "red", ai = true, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 4 }
    for minute = 1, 30 do
      t.ticks(1800)
      if not match.running() then break end
    end
    t.expect(match.running(), "the match is over").equals(false)
    t.expect(match.winner(), "the AI won").equals(2)
    t.expect((events.rts_structure_built or 0) >= 2, "the AI built").is_true()
    t.expect((events.rts_ai_attack or 0) >= 1, "the AI attacked").is_true()
    prts.log.info(string.format("AI won at minute %.1f", (S.match().ended - S.match().started) / 1800))
  end,
}
