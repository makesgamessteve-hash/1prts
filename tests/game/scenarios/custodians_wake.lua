-- Spec (campaign): The core is held; Sinking the citadels. Act VIII on the Skyfall Meres: the
-- Marshal at Caldrith's war table, the chiefs in the throne hall, the Keeper and its wardens in
-- the core. A choice made while they hold it is refused; once Caldrith's garrison has cleared
-- the core, the Marshal breaks the stone at its ledge: every citadel sinks, the Custodians
-- sleep, "The Long Calm" plays and the campaign keeps the ending. MIT License.
-- ctest-args: --start skyfall_meres
return {
  name = "the Custodians wake",
  max_seconds = 400,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Cu = require("rts.custodians")
    local Ca = require("game.campaign")
    local map = require("maps.skyfall_meres")
    prts.settings.set("campaign", nil)
    Ca.begin("act8")
    local cal = prts.entity.find("Caldrith")
    t.expect(S.data(cal).state, "woken").equals("woken")
    t.expect(S.data(cal).side, "by the Crown").equals(1)
    t.expect(Ci.aboard(cal), "the Marshal aboard").is_true()
    t.expect(S.side(Cu.SIDE).stance, "hostile Custodians").equals("hostile")
    t.expect(#S.match().custodians.ruins, "on the shores").equals(3)
    for _, name in ipairs({ "Vael Tor", "Orm Sedd" }) do
      t.expect((Ci.strength(prts.entity.find(name), Cu.SIDE)), "guardians aboard " .. name).equals(6)
    end
    t.expect((Ci.strength(cal, Cu.SIDE)), "the Keeper's guard in Caldrith's core").equals(#map.KEEPER)
    t.expect(prts.entity.find("council.urgra") ~= nil and prts.entity.find("council.salt") ~= nil, "the chiefs").is_true()
    -- the core is held: nothing can be decided
    local ok, why = map.choose("steer")
    t.expect(ok, "refused").equals(false)
    t.expect(why).equals("the core is held")
    t.expect(map.choose("nothing"), "no such choice").equals(false)
    -- the garrison fights the guardians out of the core
    t.wait_until(map.core_clear, 300)
    t.expect((Ci.strength(cal, 1)) > 0, "the Crown's garrison still stands").is_true()
    -- down to the ledge in the core, and break the stone
    local x, y, z = Ci.world(cal, 3.6, -9.55, 3.6)
    t.place(x, y, z)
    t.ticks(2)
    t.face("choice.sink")
    t.ticks(2)
    t.expect(t.target(), "the stone's ledge in reach").equals("choice.sink")
    local ending
    prts.events.on("campaign_ending", "test.custodians_wake", function(k) ending = k end)
    t.input.press("use")
    t.ticks(3)
    t.expect(ending, "the stone is broken").equals("sink")
    for _, e in ipairs(Ci.all()) do t.expect(S.data(e).state, prts.entity.name(e) .. " sinks").equals("broken") end
    t.expect(#S.units(Cu.SIDE), "the Custodians sleep").equals(0)
    t.expect(require("ui.banner").current.title).equals("The Long Calm")
    t.expect(Ca.done("act8"), "the act is done").is_true()
    t.expect(prts.settings.get("campaign.ending"), "the ending kept").equals("sink")
    t.expect(require("rts.match").winner(), "the Crown's").equals(1)
    t.expect(map.choose("share"), "decided once").equals(false)
    prts.settings.set("campaign", nil)
    prts.settings.save()
  end,
}
