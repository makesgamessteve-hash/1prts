-- Spec (localization): Switching the language; A missing translation. With German chosen on the
-- Game tab, the menus are drawn in German (umlauts included) while their ids stay English, the
-- campaign's acts are named in German, and text without a translation shows in English.
-- MIT License.
-- ctest-args: --start proving_ground
return {
  name = "the game in German",
  max_seconds = 30,
  run = function(t)
    local settings = require("game.settings")
    local menu = require("ui.menu")
    local I = require("lib.i18n")
    local options = require("ui.options")
    local codes = {}
    for _, l in ipairs(I.available()) do codes[#codes + 1] = l.code .. "=" .. l.name end
    t.expect(table.concat(codes, " "), "the languages offered").equals("en=English de=Deutsch")
    -- the Game tab's Language button cycles to German
    menu.open("options")
    options.tab = "game"
    t.ticks(2)
    t.expect(t.ui_has("English"), "English to start with").is_true()
    t.ui_click("game.language")
    t.expect(settings.game().language, "German").equals("de")
    t.expect(I.lang).equals("de")
    t.ticks(2)
    t.expect(t.ui_has("Sprache") and t.ui_has("Deutsch"), "the options in German").is_true()
    t.ui_click("options.back")
    t.ticks(2)
    t.expect(t.ui_has("Spielmenü") or t.ui_has("Optionen"), "the game menu in German").is_true()
    t.expect(t.ui_has("Weiter") and t.ui_has("Mitwirkende"), "its buttons").is_true()
    -- ids stay English: tests and bindings do not change with the language
    t.ui_click("Campaign")
    t.expect(menu.screen, "clicked by its English id").equals("campaign")
    t.ticks(2)
    t.expect(t.ui_has("Weiter: Prolog: Harrowmark"), "a composed label, translated in parts").is_true()
    t.expect(t.ui_has("I - Die stumme Wacht  (gesperrt)"), "an act's title and its state").is_true()
    -- a missing translation shows the English
    t.expect(I.t("A line nobody has translated"), "English").equals("A line nobody has translated")
    t.expect(I.t("Talk to Pell Durran"), "a prompt").equals("Mit Pell Durran sprechen")
    t.expect(I.t("Continue: {act}", { act = "X" }), "placeholders").equals("Weiter: X")
    menu.open(nil)
    settings.set_game("language", "en", false)
    t.expect(I.lang, "English again").equals("en")
    t.expect(I.t("Options"), "untranslated in English").equals("Options")
    prts.settings.save()
  end,
}
