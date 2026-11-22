-- Spec (comfort): Riding a ship in VR. Standing still in the galleon's chart room as she sails,
-- in a headset: the riding vignette darkens the view's edges; turned off, the view stays clear.
-- MIT License.
-- ctest-args: --start saltcoast
return {
  name = "the riding vignette",
  max_seconds = 60,
  run = function(t)
    local HEAD = { pos = { 0, 1.65, 0 } }
    local function frame() t.xr_frame{ head = HEAD, left = { stick = { 0, 0 } }, right = { stick = { 0, 0 } } } end
    t.ticks(30)
    t.xr_begin()
    frame()
    t.ticks(5)
    local x = prts.xr.settings()
    t.expect(x.ride_vignette, "on by default").is_true()
    t.expect(x.ride_strength, "its strength").near(0.8, 1e-6)
    t.expect(prts.xr.state().vignette, "clear while still at anchor").near(0, 1e-6)
    local galleon = prts.entity.find("ship.galleon")
    local gx, _, gz = t.pos(galleon)
    require("rts.fleet").order(galleon, gx - 300, gz)
    t.wait_until(function() return prts.xr.state().carried end, 20)
    t.ticks(30)
    t.expect(t.parent(t.commander()), "aboard").equals("ship.galleon")
    t.expect(prts.xr.state().vignette, "the riding vignette").near(0.8, 1e-3)
    prts.xr.settings{ ride_vignette = false }
    t.ticks(30)
    t.expect(prts.xr.state().vignette, "turned off: clear").near(0, 1e-3)
    t.expect(prts.xr.state().carried, "still carried").is_true()
    prts.xr.settings{ ride_vignette = true, ride_strength = 0.5 }
    t.ticks(30)
    t.expect(prts.xr.state().vignette, "a gentler strength").near(0.5, 1e-3)
    t.xr_end()
  end,
}
