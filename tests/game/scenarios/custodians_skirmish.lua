-- Spec (custodians): Hostile Custodians in a skirmish. With the option set to hostile, a
-- skirmish on Ashford Vale starts with the Custodians holding the map's two Imperium ruins,
-- each with a guard and a spire. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "hostile Custodians in a skirmish",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local Cu = require("rts.custodians")
    local settings = require("game.settings")
    local map = require("maps.ashford_vale")
    t.expect(settings.game().custodians, "off by default").equals("off")
    settings.set_game("custodians", "neutral", false)
    settings.set_game("custodians", require("ui.options").cycle(settings.CUSTODIANS, "neutral"), false)
    t.expect(settings.game().custodians, "hostile").equals("hostile")
    map.on_started(map.ID)
    t.expect(Cu.active(), "the Custodians take part").is_true()
    t.expect(S.side(Cu.SIDE).stance).equals("hostile")
    t.expect(#S.match().custodians.ruins, "both ruins").equals(#map.RUINS)
    t.expect(#S.structures(Cu.SIDE, "custodian.spire", true), "a spire at each").equals(2)
    t.expect(#S.units(Cu.SIDE), "a guard at each").equals(2 * #Cu.GUARD)
    t.ticks(31)
    t.expect(require("rts.match").running(), "the skirmish runs").is_true()
    settings.set_game("custodians", "off", false)
  end,
}
