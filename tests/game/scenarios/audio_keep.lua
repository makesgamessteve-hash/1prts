-- Spec (audio-playback): Sound to the left, Muting music, and Door closes on a noisy room.
-- Headless runs mix on the null device, so the levels heard are measured exactly.
return {
  name = "the keep's door creaks in the commander's left ear, and the forge can be muffled",
  run = function(t)
    local map = require("maps.proving_ground")
    t.expect(prts.audio.info().music ~= nil, "music plays from the start").is_true()
    t.ticks(2)
    t.expect(prts.audio.info().left > 0, "the music is heard").is_true()

    -- Muting music: only effects remain
    prts.audio.set_volume("music", 0)
    prts.audio.set_volume("ambience", 0)
    t.ticks(2)
    t.expect(prts.audio.info().left, "silent with music muted").equals(0)

    -- Sound to the left: open the door, then turn east so the door (north) is on the left
    t.place("keep_door")
    t.face("keep.door", 1.1, 1.6, 0)
    t.ticks(2)
    t.expect(t.target(), "looking at").equals("keep.door")
    t.input.press("use")
    t.ticks(1)
    t.look(-math.pi / 2, 0)
    t.ticks(3)
    local creak = prts.entity.data(prts.entity.find("keep.door")).sound
    t.expect(prts.audio.voice(creak) ~= nil, "the door creaks").is_true()
    local heard = prts.audio.info()
    t.expect(heard.left > 0.003, "audible").is_true()
    t.expect(heard.left > 3 * heard.right, "louder in the left ear").is_true()
    prts.audio.set_volume("effects", 0)
    t.ticks(2)
    t.expect(prts.audio.info().left, "effects muted").equals(0)
    prts.audio.set_volume("effects", 1)
    t.wait_until(function() return prts.audio.voice(creak) == nil end, 3)

    -- Spec: Door closes on a noisy room. From the hall, the forge is clear while its door is
    -- open; closing the door muffles it (quieter, duller), and opening it clears it again.
    local Door = require("props.door")
    local door = prts.entity.find("keep.forge.door")
    prts.audio.set_volume("ambience", 1)
    prts.audio.set_volume("effects", 0) -- only the forge and the hall's torches (ambience)
    t.place("forge_door")
    t.ticks(4)
    local forge = map.forge_sound
    t.expect(prts.audio.voice(forge).occlusion, "muffled behind the closed door").equals(1)
    local closed = prts.audio.info().left
    Door.toggle(door)
    t.ticks(4)
    t.expect(prts.audio.voice(forge).occlusion, "clear through the open door").equals(0)
    t.expect(prts.audio.voice(forge).cutoff, "no low-pass").equals(0)
    local clear = prts.audio.info().left
    t.expect(clear > 0.002, "the forge is heard at its door").is_true()
    t.expect(closed < clear * 0.6, "quieter behind the closed door").is_true()
    Door.toggle(door)
    t.ticks(4)
    t.expect(prts.audio.voice(forge).cutoff < 1000, "duller when the door closes").is_true()
    t.expect(prts.audio.info().left < clear * 0.6, "quieter when the door closes").is_true()
    Door.toggle(door)
    t.ticks(4)
    t.expect(prts.audio.info().left > clear * 0.8, "clear again when it opens").is_true()

    -- Scripts can muffle a sound themselves (on top of the walls)
    t.expect(prts.audio.muffle(forge, 1), "muffle the forge").is_true()
    t.ticks(4)
    t.expect(prts.audio.info().left < clear * 0.6, "quieter when muffled").is_true()
    t.expect(prts.audio.voice(forge).cutoff < 1000, "duller when muffled").is_true()
    prts.audio.muffle(forge, 0)
    t.ticks(4)
    t.expect(prts.audio.info().left > clear * 0.5, "clear again").is_true()
    prts.audio.set_volume("effects", 1)
    prts.audio.set_volume("music", 1)
  end,
}
