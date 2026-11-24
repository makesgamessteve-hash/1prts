-- Spec (modding-guide): The example mod works. The modding guide's example mod, Watchfire Hill:
-- its entry point ran at boot, its map is offered and plays (a match of two sides, the
-- Custodians at its ruin, a company of its own unit), its rule change holds, and its French
-- table is offered as a language. MIT License.
-- ctest-args: --mod watchfire --start watchfire_hill
return {
  name = "the example mod",
  max_seconds = 30,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local I = require("lib.i18n")
    local found = false
    for _, m in ipairs(require("game.session").maps) do found = found or m.id == "watchfire_hill" end
    t.expect(found, "its map is offered").is_true()
    t.expect(require("game.session").current, "and plays").equals("watchfire_hill")
    t.expect(require("rts.match").running(), "a match").is_true()
    t.expect(S.hq(1) ~= nil and S.hq(2) ~= nil, "two sides").is_true()
    t.expect(#S.units(1, "watchfire.lantern_guard"), "its own unit").equals(6)
    t.expect(D.unit("watchfire.lantern_guard").at, "trained at the barracks").equals("crown.barracks")
    t.expect(require("rts.custodians").active(), "the Custodians at its ruin").is_true()
    t.expect(D.HORN_RANGE, "its rule").equals(2600)
    local langs = {}
    for _, l in ipairs(I.available()) do langs[#langs + 1] = l.code end
    t.expect(table.concat(langs, " "), "its language").equals("en de fr")
    I.set("fr")
    t.expect(I.t("Quit"), "French").equals("Quitter")
    t.expect(I.t("Continue: X"), "a French pattern").equals("Continuer : X")
    I.set("de")
    t.expect(I.t("Lantern guard"), "German for its words").equals("Laternenwache")
    I.set("en")
    t.ticks(31)
  end,
}
