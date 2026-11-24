-- Spec (campaign): Steering or sharing. With the core cleared, taking the helm plays "The Crown's
-- Sky" and the Custodians stand down; (as if from the start again) calling the council plays
-- "The Concord of the Meres". Losing Caldrith loses the act. MIT License.
-- ctest-args: --start skyfall_meres
return {
  name = "the other endings",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Cu = require("rts.custodians")
    local Ca = require("game.campaign")
    local map = require("maps.skyfall_meres")
    prts.settings.set("campaign", nil)
    Ca.begin("act8")
    local cal = prts.entity.find("Caldrith")
    -- clear the core (as the garrison would)
    local d = S.data(cal)
    local keep = {}
    for _, rec in ipairs(d.garrison) do if rec.side ~= Cu.SIDE then keep[#keep + 1] = rec end end
    d.garrison = keep
    t.expect(map.core_clear(), "the core is clear").is_true()
    -- the helm by the throne, used in the throne hall
    local x, y, z = Ci.world(cal, 0, 4.45, 15.0)
    t.place(x, y, z, 0)
    t.ticks(2)
    t.face("choice.steer")
    t.ticks(2)
    t.expect(t.target(), "the helm in reach").equals("choice.steer")
    t.input.press("use")
    t.ticks(3)
    t.expect(S.match().meres.chosen, "steered").equals("steer")
    t.expect(require("ui.banner").current.title).equals("The Crown's Sky")
    t.expect(#S.units(Cu.SIDE), "the Custodians stand down").equals(0)
    t.expect(S.data(cal).state, "Caldrith still flies").equals("woken")
    t.expect(prts.settings.get("campaign.ending")).equals("steer")
    -- the concord (the choice made afresh)
    S.match().meres.chosen = nil
    Ca.result = nil
    t.expect(map.choose("share"), "shared").is_true()
    t.expect(require("ui.banner").current.title).equals("The Concord of the Meres")
    t.expect(prts.settings.get("campaign.ending")).equals("share")
    -- losing Caldrith loses the act
    S.match().meres.chosen = nil
    S.match().running = true
    Ca.result = nil
    Ci.shatter(cal, 2)
    t.ticks(31)
    t.expect(S.side(1).lost, "the Crown has lost").is_true()
    t.expect(Ca.result, "the act is lost").equals("lost")
    prts.settings.set("campaign", nil)
    prts.settings.save()
  end,
}
