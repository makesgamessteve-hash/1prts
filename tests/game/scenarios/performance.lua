-- Spec (performance): A phone starts on low; A shorter draw distance (the drawing itself is
-- checked in engine/tests/test_engine.c, gfx.windowed_engine_renders_and_recovers_from_broken_content).
-- MIT License.
return {
  name = "performance profiles",
  max_seconds = 20,
  run = function(t)
    local settings = require("game.settings")
    t.expect(prts.settings.get("game.performance"), "nothing chosen yet").equals(nil)
    t.expect(settings.performance(), "a desktop starts on high").equals("high")
    t.touch(1, 0.5, 0.5, true)
    t.ticks(1)
    t.touch(1, 0.5, 0.5, false)
    t.ticks(1)
    t.expect(settings.performance(), "a touch screen starts on low").equals("low")
    settings.apply_game()
    t.expect(prts.render.settings().draw_distance, "a short draw distance").equals(350)
    settings.set_game("performance", "medium", false)
    t.expect(prts.render.settings().draw_distance, "medium").equals(900)
    settings.set_game("performance", "high", false)
    t.expect(prts.render.settings().draw_distance, "high").equals(5000)
    t.expect(prts.render.settings{ draw_distance = 10 }.draw_distance, "clamped").equals(50)
    local st = prts.render.stats()
    t.expect(st.sprites, "headless: nothing drawn").equals(0)
    t.expect(st.entity_groups).equals(0)
  end,
}
