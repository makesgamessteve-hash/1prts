-- Spec (editor): Playing a map. A map made with the editor, played: its trees, mines and props
-- are real, its two holdfasts make a match (side 1 the player's, side 2 the AI's), its company
-- stands ready and the Custodians hold its ruin. A broken file plays as an empty map and says
-- why. MIT License.
-- ctest-args: --start custom
return {
  name = "playing a map from the editor",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local Mf = require("game.mapfile")
    local Mb = require("game.mapbuild")
    local custom = require("maps.custom")
    t.expect(custom.map.name, "no file yet: an empty map").equals("map_1.map")
    t.expect(require("rts.match").running(), "no match on an empty map").equals(false)
    prts.files.write("broken.map", "format 1\nplace castle x 0 0\n")
    prts.settings.set("editor.file", "broken.map")
    local map, name, why = custom.file()
    t.expect(why, "it says why").equals("line 2: unknown kind 'castle'")
    t.expect(#map.objects, "and plays empty").equals(0)
    prts.files.remove("broken.map")
    map = assert(Mf.parse([[
format 1
name Two Lords
place tree pine 20 20
place mine gold 30 -20
place spring aether -20 40
place prop anvil 0 10 0
place holdfast crown -120 0 0 1
place holdfast red 120 0 0 2
place company crown.men_at_arms -100 20 0 1
place company nobody.at_all 100 20 0 2
place ruin imperium 0 120
]]))
    t.expect(Mb.play(map), "two sides").equals(2)
    t.expect(Mb.match(map), "a match").is_true()
    t.expect(#S.match().resources >= 3, "trees, mines, springs to gather").is_true()
    t.expect(S.side(1).ai, "side 1 is the player's").equals(false)
    t.expect(S.side(2).ai, "side 2 the AI's").is_true()
    t.expect(S.hq(1) ~= nil and S.hq(2) ~= nil, "their holdfasts").is_true()
    t.expect(#S.units(1, "crown.men_at_arms"), "the company").equals(6)
    t.expect(#S.units(2, Mb.COMPANY), "an unknown unit is men-at-arms").equals(6)
    t.expect(require("rts.custodians").active(), "the Custodians at the ruin").is_true()
    t.expect(Mb.match(Mf.new()), "one side: no match").equals(false)
    t.ticks(31)
    t.expect(require("rts.match").running(), "the match runs").is_true()
    prts.settings.set("editor", nil)
    prts.settings.save()
  end,
}
