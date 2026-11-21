-- Lua unit tests: the audio API (prts.audio) and the keep's sounds. They run headless on the
-- null device, so no audio is mixed: voice states are read back directly. MIT License.
local Door = require("props.door")
local Lift = require("props.lift")
local map = require("maps.proving_ground")

T.case("headless runs use the null device", function()
  local info = prts.audio.info()
  T.eq(info.device, "null")
  T.eq(info.rate, 48000)
  T.is_true(info.voices >= 4, "music, forge and torches")
  T.eq(info.left, 0)
end)

T.case("bus volumes can be set and read back", function()
  for _, bus in ipairs({ "master", "effects", "voice", "music", "ambience" }) do
    T.eq(prts.audio.volume(bus), 1.0, bus)
  end
  prts.audio.set_volume("music", 0.25)
  T.near(prts.audio.volume("music"), 0.25, 1e-6, "music")
  prts.audio.set_volume("music", 5)
  T.eq(prts.audio.volume("music"), 1.0, "clamped")
  T.fails(function() prts.audio.set_volume("loud", 1) end, "unknown bus")
  T.fails(function() prts.audio.volume("loud") end, "unknown bus")
end)

T.case("sounds play at a point, on an entity or without a position", function()
  -- the listener stays at the origin looking along -z: -x is to its left
  local left = prts.audio.voice(prts.audio.play{ sound = "sounds/lever_click.wav", at = { -3, 0, 0 } })
  T.eq(left.bus, "effects")
  T.is_true(left.positional, "positional")
  T.is_true(left.left > 3 * left.right, "louder on the left")
  local right = prts.audio.voice(prts.audio.play{ sound = "sounds/lever_click.wav", at = { x = 3, y = 0, z = 0 } })
  T.is_true(right.right > 3 * right.left, "louder on the right")
  local far = prts.audio.voice(prts.audio.play{ sound = "sounds/lever_click.wav", at = { 0, 0, -300 } })
  T.eq(far.left, 0, "out of range")

  local door = prts.entity.find("keep.door")
  local h = prts.audio.play{ sound = "sounds/door_open.wav", entity = door, bus = "voice", volume = 0.5, range = 20 }
  T.eq(prts.audio.voice(h).bus, "voice")
  T.near(prts.audio.voice(h).volume, 0.5, 1e-6, "volume")

  local flat = prts.audio.play{ sound = "sounds/footstep.wav", loop = true }
  T.eq(prts.audio.voice(flat).positional, false)
  T.eq(prts.audio.voice(flat).loop, true)
  T.eq(prts.audio.voice(flat).left, prts.audio.voice(flat).right)
  T.eq(prts.audio.stop(flat, 0.1), true)
  T.eq(prts.audio.voice(flat).stopping, true)

  T.fails(function() prts.audio.play{ at = { 0, 0, 0 } } end, "no sound")
  T.fails(function() prts.audio.play{ sound = "sounds/footstep.wav", bus = "nowhere" } end, "bad bus")
  T.fails(function() prts.audio.play{ sound = "sounds/footstep.wav", at = 5 } end, "bad position")
  T.fails(function() prts.audio.play{ sound = "sounds/footstep.wav", volume = "loud" } end, "bad volume")
  T.fails(function() prts.audio.play{ sound = "sounds/footstep.wav", entity = 123456789 } end, "stale entity")
  T.fails(function() prts.audio.play("sounds/footstep.wav") end, "options table")
end)

T.case("missing and broken sounds play a placeholder beep", function()
  T.eq(prts.audio.voice(prts.audio.play{ sound = "sounds/no_such_sound.wav" }).placeholder, true, "missing")
  T.eq(prts.audio.voice(prts.audio.play{ sound = "sounds/no_such_sound.wav" }).placeholder, true, "cached")
  T.eq(prts.audio.voice(prts.audio.play{ sound = "scripts/main.lua" }).placeholder, true, "not audio")
  T.eq(prts.audio.voice(prts.audio.play{ sound = "../escape.wav" }).placeholder, true, "invalid path")
  T.eq(prts.audio.voice(prts.audio.play{ sound = "sounds/footstep.wav" }).placeholder, false, "a real sound")
end)

T.case("muffling makes a sound quieter and duller", function()
  local h = prts.audio.play{ sound = "sounds/forge_loop.wav", at = { 0, 0, -3 }, loop = true }
  local clear = prts.audio.voice(h)
  T.eq(clear.cutoff, 0, "no filter")
  T.eq(prts.audio.muffle(h, 1), true)
  local muffled = prts.audio.voice(h)
  T.is_true(muffled.left < clear.left, "quieter")
  T.is_true(muffled.cutoff > 0 and muffled.cutoff < 1000, "low cutoff")
  T.eq(prts.audio.muffle(h, 0.5), true)
  T.is_true(prts.audio.voice(h).cutoff > muffled.cutoff, "half muffled is brighter")
  T.eq(prts.audio.muffle(h, 7), true)
  T.eq(prts.audio.voice(h).muffle, 1, "clamped")
  T.eq(prts.audio.stop(h), true)
  -- finished or missing handles are harmless
  T.eq(prts.audio.muffle(nil, 1), false)
  T.eq(prts.audio.stop(nil), false)
  T.eq(prts.audio.stop(0), false)
  T.eq(prts.audio.voice(987654), nil)
  T.fails(function() prts.audio.muffle(h) end, "amount is required")
end)

T.case("music cross-fades between streamed tracks", function()
  local first = prts.audio.info().music -- main.lua started the Proving Ground theme
  T.is_true(first ~= nil, "music at boot")
  T.eq(prts.audio.voice(first).bus, "music")
  T.eq(prts.audio.voice(first).placeholder, false, "the track loaded")
  T.eq(prts.audio.music{ track = "music/proving_ground.ogg", volume = 0.8 }, first, "same track keeps playing")
  T.near(prts.audio.voice(first).volume, 0.8, 1e-6, "volume")
  local menu = prts.audio.music{ track = "music/menu.ogg", fade = 2 }
  T.is_true(menu ~= nil and menu ~= first, "a new voice")
  T.eq(prts.audio.voice(first).stopping, true, "the old track fades out")
  T.eq(prts.audio.voice(menu).fade, 0, "the new track fades in")
  T.eq(prts.audio.info().music, menu)
  T.eq(prts.audio.music{ fade = 0.5 }, nil, "no track: silence")
  T.eq(prts.audio.info().music, nil)
  T.eq(prts.audio.voice(menu).stopping, true)
  local beep = prts.audio.music{ track = "music/missing.ogg", fade = 0 }
  T.eq(prts.audio.voice(beep).loop, false, "a missing track beeps once")
  T.eq(prts.audio.voice(beep).placeholder, true)
  local broken = prts.audio.music{ track = "sounds/footstep.wav" }
  T.is_true(broken ~= nil, "not a vorbis stream")
  T.fails(function() prts.audio.music("music/menu.ogg") end, "options table")
  T.fails(function() prts.audio.music{ track = "music/menu.ogg", fade = "slow" } end, "bad fade")
  prts.audio.music{ track = "music/proving_ground.ogg", fade = 0 }
end)

T.case("the keep's forge and torches loop on the ambience bus", function()
  local forge = prts.audio.voice(map.forge_sound)
  T.eq(forge.bus, "ambience")
  T.eq(forge.loop, true)
  T.eq(forge.placeholder, false, "the forge sound loaded")
  T.eq(#map.torch_sounds, 2)
  for _, h in ipairs(map.torch_sounds) do
    T.eq(prts.audio.voice(h).bus, "ambience")
    T.eq(prts.audio.voice(h).placeholder, false, "the torch sound loaded")
  end
end)

T.case("every sound the props use exists", function()
  for _, set in ipairs({ Door.SOUNDS, Lift.SOUNDS }) do
    for name, path in prts.sorted_pairs(set) do
      local h = prts.audio.play{ sound = path, volume = 0 }
      T.eq(prts.audio.voice(h).placeholder, false, name)
      prts.audio.stop(h)
    end
  end
end)

T.case("doors and the lift make sounds", function()
  -- every test file shares one world: leave the door closed and the lift where it is
  local d = prts.entity.find("keep.door")
  T.eq(Door.toggle(d), true)
  local sound = prts.entity.data(d).sound
  T.is_true(prts.audio.voice(sound) ~= nil, "door sound")
  T.eq(prts.audio.voice(sound).positional, true)
  T.eq(prts.audio.voice(sound).placeholder, false, "the open sound loaded")
  T.eq(Door.toggle(d), false)
  T.is_true(prts.entity.data(d).sound ~= sound, "closing plays another sound")
  T.eq(prts.audio.voice(prts.entity.data(d).sound).placeholder, false, "the close sound loaded")
  local state = prts.entity.data(map.lift)
  local before = prts.audio.info().voices
  T.eq(Lift.on_use(state.calls[state.current]), false, "the lift is already here")
  T.eq(prts.audio.info().voices, before + 1, "lever click")
  T.eq(Lift.on_move_done(map.lift), true)
  T.eq(prts.audio.info().voices, before + 2, "arrival clank")
end)
