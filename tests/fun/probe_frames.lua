-- Fun audit: the windowed pass (tools/fun/audit.py runs it under a display with --front-end):
-- time from the menu's map button to control, and frames for the visual audit: each view is
-- taken with the HUD and subtitles drawn and without them, so the audit can find the text and
-- its backing panel by difference and measure their contrast. MIT License.
-- ctest-args: --front-end
return {
  name = "fun audit frames",
  max_seconds = 120,
  run = function(t)
    local Tel = require("fun.telemetry")
    Tel.install(t)
    t.ticks(2)
    Tel.emit("menu", {})
    t.screenshot("boot_menu")
    Tel.emit("click", {})
    t.ui_click("Harrowmark")
    local c = t.commander()
    local x0, _, z0 = t.pos(c)
    t._hold("move_forward", 4)
    local moved = false
    for _ = 1, 30 do
      t.ticks(1)
      local x, _, z = t.pos(c)
      if math.abs(x - x0) + math.abs(z - z0) > 1e-3 then moved = true break end
    end
    Tel.emit("control", { moved = moved })
    t._hold("move_forward", 0)
    t.ticks(5) -- standing still: the two frames of a pair must differ only by the overlay
    local hud, subtitles = require("ui.hud"), require("ui.subtitles")
    local function pair(name)
      t.ticks(1)
      t.screenshot(name .. "_on")
      hud.visible, subtitles.visible = false, false
      t.ticks(1)
      t.screenshot(name .. "_off")
      hud.visible, subtitles.visible = true, true
      Tel.emit("frame_pair", { name = name })
    end
    -- the prologue's first line on the road at dusk, with the King's Gate ahead
    pair("road_prologue")
    -- the prompt on the keep door
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    pair("keep_door_prompt")
    -- the castellan speaking in the great hall
    t.place(2.8, 0.2, -6.5)
    t.face("npc.hall_guard", 0, 1.3, 0)
    t.ticks(2)
    t._hold("use", 1)
    t.ticks(2)
    t.wait_until(function() return not prts.entity.moving(prts.entity.find("npc.hall_guard")) end, 3)
    t.ticks(2)
    pair("castellan_talks")
    -- the winch prompt in the King's Gate, the evening sky through the arch
    local map = require("maps.proving_ground")
    local winch = prts.entity.data(map.kings_gate).winch
    local wx, _, wz = prts.entity.pos(winch)
    t.place(wx - 1.0, 0, wz - 0.4)
    t.face(prts.entity.name(winch))
    t.ticks(2)
    pair("winch_prompt")
    -- the end of an episode: the top of the Raven Tower
    t.place(0, map.RAVEN.height, map.RAVEN.z + 4.5, 0, 0.1)
    t.ticks(2)
    t.screenshot("raven_top")
  end,
}
