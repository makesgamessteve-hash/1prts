-- Spec (reef-war): Sinking the Tidehold. The Crown's galleon and caravel sail out past the reef
-- and bombard the Tidehold until it sinks; the Crown wins. MIT License.
-- ctest-labels: slow
-- ctest-args: --start reefholm
return {
  name = "the Reef War",
  max_seconds = 900,
  run = function(t)
    local S = require("rts.state")
    local Fl = require("rts.fleet")
    local match = require("rts.match")
    local th = S.hq(2)
    local tx, _, tz = t.pos(th)
    -- the League's ships are kept busy elsewhere (the test is about the raft-city)
    for _, name in ipairs({ "ship.brigantine", "ship.sloop.1", "ship.sloop.2" }) do Fl.gone(prts.entity.find(name)) end
    Fl.order(prts.entity.find("ship.galleon"), tx - 60, tz - 70)
    Fl.order(prts.entity.find("ship.caravel"), tx + 60, tz - 70)
    t.wait_until(function() return not S.alive(th) end, 800)
    t.ticks(31)
    t.expect(match.winner(), "the Crown wins the Reef War").equals(1)
  end,
}
