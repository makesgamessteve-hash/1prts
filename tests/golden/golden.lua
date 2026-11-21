-- Render golden images (task 7.9). Fixed views of the Proving Ground for the resolution
-- presets, each effect toggle, a field map in front of a 20 km horizon, a directional sprite
-- from three sides, a door that hides a room, and the live war table.
-- tools/golden/run.cmake renders this at 640x360 and compares the shots with tests/golden/ref/;
-- tools/golden/update refreshes the references. Needs a window (skipped shots when headless).
-- MIT License.
local DEFAULT = { preset = "retro", dither = true, snap = true, affine = false, fog = true }

local function settings(over)
  local s = {}
  for k, v in pairs(DEFAULT) do s[k] = v end
  for k, v in pairs(over or {}) do s[k] = v end
  prts.render.settings(s)
end

return {
  name = "render golden images",
  max_seconds = 60,
  run = function(t)
    local function shot(name, over)
      settings(over)
      t.ticks(2)
      t.screenshot(name)
    end

    -- Spec: Changing the preset; Dusk lighting
    -- far down the road, so the castle stands in the dusk haze (fog starts at 90 m)
    t.place(0, prts.scene.height(0, 250), 250, 0, 0.06)
    shot("approach_authentic", { preset = "authentic" })
    shot("approach_retro")
    shot("approach_crisp", { preset = "crisp" })
    shot("approach_no_fog", { fog = false })

    -- Spec: Turning off dithering (and the other run-time toggles)
    t.place("hall_view", -0.05)
    shot("hall_default")
    shot("hall_no_dither", { dither = false })
    shot("hall_affine", { affine = true })
    shot("hall_no_snap", { snap = false })
    settings()

    -- Spec: Directional sprite. A guard of its own on open ground, facing -z, seen from the
    -- front, the side and the back.
    local gx, gz = 14, 30
    local gy = prts.scene.height(gx, gz)
    prts.scene.sprite{ sprite = "sprites/crown_guard.lua", pos = { gx, gy, gz }, yaw = 0 }
    for i, side in ipairs({ "front", "side", "back" }) do
      local a = (i - 1) * math.pi / 2
      t.place(gx - math.sin(a) * 2.6, gy, gz - math.cos(a) * 2.6)
      t.face(gx, gy + 1.1, gz)
      t.ticks(2)
      t.screenshot("guard_" .. side)
    end

    -- Spec: Closed door hides a room
    t.place("forge_door")
    t.face("keep.forge.door", 0.6, 1.4, 0)
    t.ticks(2)
    if not t.headless() then
      t.expect(t.drawn_cells()["keep.forge"], "forge drawn behind the closed door").equals(nil)
    end
    t.screenshot("forge_door_closed")
    t.input.press("use")
    t.ticks(30)
    if not t.headless() then
      t.expect(t.drawn_cells()["keep.forge"], "forge drawn through the open door").is_true()
    end
    t.screenshot("forge_door_open")

    -- Spec: Live war table
    t.place(-1.4, 9.2, 5.0, -0.25, -0.62)
    t.ticks(3)
    t.screenshot("war_table")

    -- An in-world UI surface: the notice board in the great hall
    t.place(3.2, 0.2, -4.6)
    t.face(5.38, 1.95, -4.6)
    t.ticks(3)
    if not t.headless() then
      t.expect(t.ui_has("Orders of the day", "hall.notices"), "the board's title is drawn").is_true()
    end
    t.screenshot("notice_board")

    -- Spec: Field map and horizon in one view. A placeholder map held 0.4 m in front of the
    -- eyes, and a mountain ridge 20 km away, drawn in the same view without fog.
    t.place(0, 0, 160, math.pi, -0.35) -- on the road outside the castle, looking south
    local _, _, eye = prts.commander.view()
    prts.scene.box{ min = { -0.2, eye - 0.17, 160.3 }, max = { 0.2, eye - 0.16, 160.6 }, material = "rug", solid = false }
    prts.scene.box{ min = { -6000, -200, 20000 }, max = { 6000, 2600, 21500 }, material = "rock", solid = false,
                    tile = 1000 }
    shot("field_map_and_horizon", { fog = false })
    settings()
  end,
}
