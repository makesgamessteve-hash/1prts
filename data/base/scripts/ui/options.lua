-- The options menu: video (retro resolution preset and each retro effect), audio volumes and
-- controls. Video changes apply at once, without a restart, and are saved. MIT License.
local settings = require("game.settings")
local controls = require("ui.controls")

local O = { tab = "video" }

O.TABS = {
  { id = "video", label = "Video" },
  { id = "audio", label = "Audio" },
  { id = "controls", label = "Controls" },
}
O.EFFECTS = {
  { key = "dither", label = "Palette and dithering" },
  { key = "snap", label = "Vertex snapping" },
  { key = "affine", label = "Affine textures" },
  { key = "fog", label = "Distance fog" },
  { key = "crosshair", label = "Crosshair" },
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
    prts.ui.row({ 80, 80, 80, -1 })
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
    else
      controls.draw()
    end
  end)
end

return O
