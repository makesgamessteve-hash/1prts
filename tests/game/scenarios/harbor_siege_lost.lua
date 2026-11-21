-- Spec (harbor-siege): The galleon lost. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "the Harbor Siege lost with the galleon",
  max_seconds = 60,
  run = function(t)
    local match = require("rts.match")
    local Fl = require("rts.fleet")
    t.ticks(30)
    Fl.sink(prts.entity.find("ship.galleon"))
    t.wait_until(function() return not match.running() end, 20)
    t.expect(match.winner(), "lost").equals(2)
  end,
}
