-- Spec (reef-war): The harbor falls. MIT License.
-- ctest-args: --start reefholm
return {
  name = "the harbour falls",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local match = require("rts.match")
    require("rts.structures").damage(S.hq(1), 1e7)
    t.ticks(31)
    t.expect(match.winner(), "the League wins").equals(2)
  end,
}
