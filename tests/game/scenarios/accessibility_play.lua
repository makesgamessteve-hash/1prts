-- Spec (accessibility): Toggle crouch; Brightness under the ground; Faster game. Under
-- Merewatch Hill: one press of crouch keeps the commander crouched until the next; with the
-- brightness boost the tunnels' fog reaches twice as far and is lighter; game speed 2.
-- MIT License.
-- ctest-args: --start underhill
return {
  name = "toggles, brightness and game speed",
  max_seconds = 120,
  run = function(t)
    local settings = require("game.settings")
    local Fog = require("lib.fog")
    local Tu = require("rts.tunnels")
    local c = t.commander()
    t.ticks(5)
    local function eye() local _, _, e = prts.commander.view() return e end
    local standing = eye()
    settings.set_game("toggle_crouch", true, false)
    t.expect(prts.input.toggle("crouch"), "crouch is a toggle").is_true()
    t.input.press("crouch")
    t.ticks(30)
    t.expect(eye() < standing - 0.3, "crouched after one press").is_true()
    t.ticks(30)
    t.expect(eye() < standing - 0.3, "and still crouched").is_true()
    t.input.press("crouch")
    t.ticks(30)
    t.expect(eye(), "standing again").near(standing, 0.05)
    settings.set_game("toggle_crouch", false, false)
    t.expect(prts.input.toggle("crouch"), "a hold again").equals(false)
    -- into the tunnels: the dark fog, then with the boost
    local g = Tu.data().nodes.j1
    t.place(g.x + 4, g.y + 0.2, g.z)
    t.ticks(5)
    local near, far, col, layer = Fog.current()
    t.expect(layer, "the dark of the tunnels").equals("underground")
    settings.set_game("dark_boost", true, false)
    local near2, far2, col2 = Fog.current()
    t.expect(far2, "twice as far").near(far * 2, 1e-4)
    t.expect(col2[1] > col[1] and col2[2] > col[2], "lighter").is_true()
    settings.set_game("dark_boost", false, false)
    t.expect(select(2, Fog.current()), "back").near(far, 1e-4)
    -- game speed (each tick the same; engine/tests/test_engine.c counts the ticks)
    settings.set_game("speed", 2.0, false)
    t.expect(prts.time.scale(), "twice as fast").equals(2.0)
    settings.set_game("speed", 1.0, false)
    t.expect(prts.time.scale(), "normal").equals(1.0)
  end,
}
