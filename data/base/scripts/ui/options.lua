-- The options menu: video (retro resolution preset and each retro effect), audio volumes,
-- controls, VR (comfort options and the video settings in a headset) and game (the prologue).
-- Video and VR changes apply at once, without a restart, and are saved. MIT License.
local settings = require("game.settings")
local controls = require("ui.controls")

local O = { tab = "video" }

O.TABS = {
  { id = "video", label = "Video" },
  { id = "audio", label = "Audio" },
  { id = "controls", label = "Controls" },
  { id = "vr", label = "VR" },
  { id = "game", label = "Game" },
  { id = "access", label = "Access" },
}
O.EFFECTS = {
  { key = "dither", label = "Palette and dithering" },
  { key = "snap", label = "Vertex snapping" },
  { key = "affine", label = "Affine textures" },
  { key = "fog", label = "Distance fog" },
  { key = "crosshair", label = "Crosshair" },
}

O.GAME_OPTIONS = {
  { key = "prologue", label = "Prologue when a map starts from the menu" },
}

O.XR_OPTIONS = {
  { key = "vignette", label = "Comfort vignette while moving" },
  { key = "ride_vignette", label = "Comfort vignette while riding and flying" },
  { key = "seated", label = "Seated mode (standing eye height)" },
}
-- M9 accessibility (the Access tab): checkboxes, and choices cycled by a button.
O.ACCESS_OPTIONS = {
  { key = "toggle_crouch", label = "Crouch is a toggle" },
  { key = "toggle_run", label = "Run is a toggle" },
  { key = "dark_boost", label = "Brighter tunnels, caverns and deep water" },
}

-- The next value after `current` in `list` (round).
function O.cycle(list, current)
  for i, v in ipairs(list) do
    if v == current then return list[i % #list + 1] end
  end
  return list[1]
end

function O.draw_access()
  local g = settings.game()
  local tc = settings.touch()
  prts.ui.row({ 150, -1 })
  prts.ui.label("War table colours")
  if prts.ui.button(settings.PALETTE_NAMES[g.palette] or g.palette, "game.palette") then
    settings.set_game("palette", O.cycle(settings.PALETTES, g.palette))
  end
  prts.ui.label("Map text")
  if prts.ui.button(g.map_text == 2 and "Large" or "Normal", "game.map_text") then
    settings.set_game("map_text", g.map_text == 2 and 1 or 2)
  end
  prts.ui.label("Game speed")
  if prts.ui.button(string.format("%.1fx", g.speed), "game.speed") then
    settings.set_game("speed", O.cycle(settings.SPEEDS, g.speed))
  end
  prts.ui.label("Performance")
  local perf = settings.performance()
  if prts.ui.button(perf:sub(1, 1):upper() .. perf:sub(2), "game.performance") then
    settings.set_game("performance", O.cycle(settings.PERFORMANCE_ORDER, perf))
  end
  prts.ui.label("Touch buttons")
  local v, changed = prts.ui.slider("touch.scale", tc.scale, 0.75, 1.5, 0.25)
  if changed then settings.set_touch("scale", v) end
  prts.ui.row({ -1 })
  local left, flipped = prts.ui.checkbox("Touch: left-handed", tc.left_handed, "touch.left_handed")
  if flipped then settings.set_touch("left_handed", left) end
  for _, o in ipairs(O.ACCESS_OPTIONS) do
    local value, changed2 = prts.ui.checkbox(o.label, g[o.key], "game." .. o.key)
    if changed2 then settings.set_game(o.key, value) end
  end
end

O.XR_EFFECTS = {
  { key = "snap", label = "Vertex snapping in VR" },
  { key = "dither", label = "Palette and dithering in VR" },
  { key = "affine", label = "Affine textures in VR" },
}

function O.cancel_capture() return controls.cancel() end

function O.draw_video()
  local v = settings.video()
  prts.ui.row({ 120, -1 })
  prts.ui.label("Resolution")
  if prts.ui.button(settings.PRESET_NAMES[v.preset], "video.preset") then
    settings.set_video("preset", settings.next_preset(v.preset))
  end
  prts.ui.row({ -1 })
  for _, fx in ipairs(O.EFFECTS) do
    local value, changed = prts.ui.checkbox(fx.label, v[fx.key], "video." .. fx.key)
    if changed then settings.set_video(fx.key, value) end
  end
  prts.ui.row({ 120, -1 })
  prts.ui.label("Field of view")
  local fov, changed = prts.ui.slider("video.fov", v.fov, 50, 110, 1)
  if changed then settings.set_video("fov", fov, false) end
end

function O.draw_vr()
  local x, xv = settings.xr(), settings.xr_video()
  prts.ui.row({ 120, -1 })
  prts.ui.label("Movement")
  if prts.ui.button(x.locomotion == "teleport" and "Teleport" or "Smooth", "xr.locomotion") then
    settings.set_xr("locomotion", x.locomotion == "teleport" and "smooth" or "teleport")
  end
  prts.ui.label("Turning")
  if prts.ui.button(x.turn == "smooth" and "Smooth" or "Snap", "xr.turn") then
    settings.set_xr("turn", x.turn == "smooth" and "snap" or "smooth")
  end
  if x.turn == "smooth" then
    prts.ui.label("Turn speed")
    local v, changed = prts.ui.slider("xr.turn_speed", x.turn_speed, 30, 360, 10)
    if changed then settings.set_xr("turn_speed", v, false) end
  else
    prts.ui.label("Snap angle")
    local v, changed = prts.ui.slider("xr.snap_angle", x.snap_angle, 15, 90, 15)
    if changed then settings.set_xr("snap_angle", v, false) end
  end
  prts.ui.label("Resolution in VR")
  if prts.ui.button(settings.PRESET_NAMES[xv.preset], "xr.preset") then
    settings.set_xr("preset", settings.next_preset(xv.preset), true, true)
  end
  prts.ui.row({ -1 })
  for _, o in ipairs(O.XR_OPTIONS) do
    local value, changed = prts.ui.checkbox(o.label, x[o.key], "xr." .. o.key)
    if changed then settings.set_xr(o.key, value) end
  end
  for _, fx in ipairs(O.XR_EFFECTS) do
    local value, changed = prts.ui.checkbox(fx.label, xv[fx.key], "xr.video." .. fx.key)
    if changed then settings.set_xr(fx.key, value, true, true) end
  end
end

function O.draw_game()
  local g = settings.game()
  prts.ui.row({ -1 })
  for _, o in ipairs(O.GAME_OPTIONS) do
    local value, changed = prts.ui.checkbox(o.label, g[o.key], "game." .. o.key)
    if changed then settings.set_game(o.key, value) end
  end
  prts.ui.row({ 160, -1 })
  prts.ui.label("Language")
  local I = require("lib.i18n")
  local langs, name = I.available(), g.language
  for _, l in ipairs(langs) do if l.code == g.language then name = l.name end end
  if prts.ui.button(name, "game.language") then
    local codes = {}
    for _, l in ipairs(langs) do codes[#codes + 1] = l.code end
    settings.set_game("language", O.cycle(codes, g.language))
  end
  prts.ui.label("Custodians in skirmish")
  if prts.ui.button(settings.CUSTODIAN_NAMES[g.custodians] or g.custodians, "game.custodians") then
    settings.set_game("custodians", O.cycle(settings.CUSTODIANS, g.custodians))
  end
  prts.ui.label("Camera motion (bob, dips, shake)")
  local v, moved = prts.ui.slider("game.camera_motion", g.camera_motion, 0, 1, 0.25)
  if moved then
    settings.set_game("camera_motion", v)
    prts.commander.set_feel{ motion = v }
  end
end

function O.draw_audio()
  prts.ui.row({ -1 })
  if not settings.audio_api() then prts.ui.label("No audio mixer in this build; volumes are kept for later.") end
  prts.ui.row({ 120, -1 })
  for _, bus in ipairs(settings.BUSES) do
    prts.ui.label(settings.BUS_NAMES[bus])
    local v, changed = prts.ui.slider("audio." .. bus, settings.audio_volume(bus), 0, 1, 0.05)
    if changed then settings.set_audio(bus, v) end
  end
end

-- back() leaves the options (the menu decides where to).
function O.draw(w, h, back)
  local ww, wh = math.min(440, w - 16), math.min(290, h - 16)
  prts.ui.window("Options", math.floor((w - ww) / 2), math.floor((h - wh) / 2), ww, wh, function()
    prts.ui.row({ 60, 60, 60, 60, 60, 60, -1 })
    for _, tab in ipairs(O.TABS) do
      if prts.ui.button(tab.id == O.tab and ("[" .. tab.label .. "]") or tab.label, "tab." .. tab.id) then
        O.tab = tab.id
        controls.cancel()
      end
    end
    if prts.ui.button("Back", "options.back") then
      controls.cancel()
      prts.settings.save()
      back()
    end
    if O.tab == "video" then
      O.draw_video()
    elseif O.tab == "audio" then
      O.draw_audio()
    elseif O.tab == "vr" then
      O.draw_vr()
    elseif O.tab == "game" then
      O.draw_game()
    elseif O.tab == "access" then
      O.draw_access()
    else
      controls.draw()
    end
  end)
end

return O
