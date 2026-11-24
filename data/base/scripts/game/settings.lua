-- Player settings with their defaults: video (retro preset and effects), audio volumes, VR
-- (comfort options and the video settings used in a headset) and game (the prologue).
-- Values live in config.lua in the user area (prts.settings); only changes are stored, and
-- they are applied at boot and whenever the options menu changes them. MIT License.
local M = {}

M.VIDEO = { preset = "retro", dither = true, snap = true, affine = false, fog = true, crosshair = true, fov = 70 }
M.PRESETS = { "authentic", "retro", "crisp", "hd", "native" }
M.PRESET_NAMES = {
  authentic = "Authentic (180 lines)", retro = "Retro (270 lines)", crisp = "Crisp (360 lines)",
  hd = "HD (540 lines)", native = "Native resolution",
}
-- VR. Spec "Comfort defaults": in a headset vertex snapping, affine textures, dithering and the
-- crosshair are off and the comfort vignette is on until the player changes them.
M.XR = { locomotion = "smooth", turn = "snap", snap_angle = 45, turn_speed = 120, vignette = true, seated = false,
         ride_vignette = true, ride_strength = 0.8 }
M.XR_VIDEO = { preset = "hd", dither = false, snap = false, affine = false, crosshair = false }
M.BUSES = { "master", "music", "effects", "voice" }
M.BUS_NAMES = { master = "Master", music = "Music", effects = "Effects", voice = "Voices" }

-- The current video settings: saved values over the defaults (wrong types are ignored).
function M.video()
  local v = {}
  for _, key in ipairs({ "preset", "dither", "snap", "affine", "fog", "crosshair", "fov" }) do
    local saved = prts.settings.get("video." .. key)
    v[key] = M.VIDEO[key]
    if type(saved) == type(M.VIDEO[key]) then v[key] = saved end
  end
  return v
end

function M.apply_video()
  local v = M.video()
  prts.render.settings(v)
  return v
end

-- Changes one video setting, applies it at once and (unless save is false) saves it.
function M.set_video(key, value, save)
  prts.settings.set("video." .. key, value)
  M.apply_video()
  if save ~= false then prts.settings.save() end
end

-- Saved values under prefix over the defaults (wrong types are ignored).
local function read(prefix, defaults)
  local v = {}
  for key, default in pairs(defaults) do
    local saved = prts.settings.get(prefix .. key)
    v[key] = default
    if type(saved) == type(default) then v[key] = saved end
  end
  return v
end

-- The VR settings: comfort options (xr.<key>) and video in the headset (xr.video.<key>).
function M.xr() return read("xr.", M.XR) end
function M.xr_video() return read("xr.video.", M.XR_VIDEO) end

-- Game settings (game.<key>): whether a map started from the front-end menu opens with its
-- prologue (a few spoken lines as the Marshal arrives), and how much the view moves on its own
-- (camera_motion: head bob, landing dips and shakes; 0 off .. 1 full; never in VR).
-- M9 accessibility and performance: the war maps' palette and text size, crouch and run as
-- toggles, a brightness boost in dark places, game speed, and the performance profile (nil:
-- low on touch screens and in headsets, high elsewhere, until the player picks one).
M.GAME = { prologue = true, camera_motion = 1.0, palette = "default", map_text = 1, toggle_crouch = false,
           toggle_run = false, dark_boost = false, speed = 1.0, custodians = "off", language = "en" }
-- M10: the Custodians in skirmish (off, neutral or hostile).
M.CUSTODIANS = { "off", "neutral", "hostile" }
M.CUSTODIAN_NAMES = { off = "Off", neutral = "Neutral", hostile = "Hostile" }
M.PALETTES = { "default", "deuteranopia", "protanopia", "tritanopia", "high_contrast" }
M.PALETTE_NAMES = { default = "Default", deuteranopia = "Deuteranopia", protanopia = "Protanopia",
                    tritanopia = "Tritanopia", high_contrast = "High contrast" }
M.SPEEDS = { 0.5, 1.0, 1.5, 2.0 }
M.PERFORMANCE = { low = { draw_distance = 350, preset = "retro" }, medium = { draw_distance = 900 },
                  high = { draw_distance = 5000 } }
M.PERFORMANCE_ORDER = { "low", "medium", "high" }
function M.game() return read("game.", M.GAME) end

-- The performance profile: the saved one, or low on touch screens and in headsets.
function M.performance()
  local p = prts.settings.get("game.performance")
  if type(p) == "string" and M.PERFORMANCE[p] then return p end
  local scheme = prts.input.scheme()
  return (scheme == "touch" or scheme == "xr") and "low" or "high"
end

-- Applies the M9 game options: toggles, speed, the dark boost, the profile's draw distance.
function M.apply_game()
  local g = M.game()
  require("lib.i18n").set(g.language) -- M10: the language the game's text is shown in
  prts.input.toggle("crouch", g.toggle_crouch)
  prts.input.toggle("run", g.toggle_run)
  prts.time.scale(g.speed)
  require("lib.fog").boost(g.dark_boost)
  local p = M.PERFORMANCE[M.performance()]
  local v = { draw_distance = p.draw_distance }
  if p.preset and prts.settings.get("video.preset") == nil then v.preset = p.preset end
  prts.render.settings(v)
  return g
end

-- Touch controls (M9): the buttons' size and the left-handed mirror.
M.TOUCH = { scale = 1.0, left_handed = false }
function M.touch() return read("touch.", M.TOUCH) end
function M.set_touch(key, value, save)
  prts.settings.set("touch." .. key, value)
  require("ui.touch").apply()
  if save ~= false then prts.settings.save() end
end

-- Changes one game setting and (unless save is false) saves it; it is read when it matters.
function M.set_game(key, value, save)
  prts.settings.set("game." .. key, value)
  M.apply_game()
  if save ~= false then prts.settings.save() end
end

function M.apply_xr()
  local v = M.xr()
  for key, value in pairs(M.xr_video()) do v[key] = value end
  return prts.xr.settings(v)
end

-- Changes one VR setting (video ones with video = true), applies it and (unless save is false) saves it.
function M.set_xr(key, value, save, video)
  prts.settings.set((video and "xr.video." or "xr.") .. key, value)
  M.apply_xr()
  if save ~= false then prts.settings.save() end
end

function M.next_preset(current)
  for i, p in ipairs(M.PRESETS) do
    if p == current then return M.PRESETS[i % #M.PRESETS + 1] end
  end
  return M.PRESETS[1]
end

function M.audio_volume(bus)
  local v = prts.settings.get("audio." .. bus)
  return type(v) == "number" and v or 1.0
end

-- The audio module's bus volume function, if this build has one.
function M.audio_api()
  local audio = prts.audio
  if not audio then return nil end
  return audio.set_volume or audio.set_bus_volume or audio.bus_volume
end

local function send_volume(bus, v)
  local set = M.audio_api()
  if set then pcall(set, bus, v) end
  return set ~= nil
end

function M.apply_audio()
  local applied = false
  for _, bus in ipairs(M.BUSES) do applied = send_volume(bus, M.audio_volume(bus)) end
  return applied
end

-- Volume sliders change continuously; the options menu saves when it is left.
function M.set_audio(bus, v)
  prts.settings.set("audio." .. bus, v)
  return send_volume(bus, v)
end

function M.apply()
  M.apply_video()
  M.apply_audio()
  M.apply_xr()
  M.apply_game()
end

return M
