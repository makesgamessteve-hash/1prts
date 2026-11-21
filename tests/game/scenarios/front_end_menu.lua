-- Spec (lua-scripting): Script-driven boot, the front-end menu at start-up.
-- Spec (asset-filesystem): Credits screen (base game and every enabled mod).
-- Spec (retro-rendering): Changing the preset (in the video options, without a restart).
-- ctest-args: --front-end --mod credits_demo
return {
  name = "front-end menu: video options, credits, then the Proving Ground",
  run = function(t)
    local settings = require("game.settings")
    t.ticks(2)
    t.expect(t.commander(), "no commander before a map is chosen").equals(0)
    t.expect(prts.ui.modal(), "the menu has the pointer").is_true()
    t.expect(t.ui_has("Proving Ground"), "the map is offered").is_true()

    -- video options: the preset applies at once
    t.ui_click("Options")
    t.expect(t.ui_has("Resolution"), "video options").is_true()
    for _ = 1, #settings.PRESETS do
      if prts.render.settings().preset == "authentic" then break end
      t.ui_click("video.preset")
    end
    t.expect(prts.render.settings().lines, "authentic preset lines").equals(180)
    t.expect(t.ui_has("Authentic (180 lines)"), "preset shown").is_true()
    local dither = prts.render.settings().dither
    t.ui_click("video.dither")
    t.expect(prts.render.settings().dither, "dithering toggled").equals(not dither)
    t.ui_click("video.dither")
    t.ui_click("options.back")

    -- credits: entries of the base game, of the enabled mod and the third-party code
    t.ui_click("Credits")
    t.expect(t.ui_has("Stone wall texture - The 1stPRTS Contributors (CC-BY-4.0)"), "base entry").is_true()
    t.expect(t.ui_has("1stPRTS UI pixel font (atlas)"), "the font is credited").is_true()
    t.expect(t.ui_has("Mod: credits_demo"), "mod section").is_true()
    t.expect(t.ui_has("Demo heraldry banner - A Test Modder (CC-BY-4.0)"), "mod entry").is_true()
    t.expect(t.ui_has("microui 2.02 - MIT"), "third-party code").is_true()
    t.ui_click("credits.back")

    -- starting the map closes the menu and gives the controls to the commander
    t.ui_click("Proving Ground")
    t.ticks(2)
    t.expect(t.commander() ~= 0, "commander spawned").is_true()
    t.expect(prts.ui.modal(), "menu closed").equals(false)
    t.expect(t.ui_has("Main menu"), "no menu drawn").equals(false)

    -- leave the settings as they were for the next run
    prts.settings.set("video", nil)
    prts.settings.save()
  end,
}
