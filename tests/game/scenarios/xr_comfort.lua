-- Spec (xr-presentation): Comfort defaults, and the requirement "Comfort options". Entering
-- XR for the first time (a fresh user area) uses no vertex snapping and turns the comfort
-- vignette on; smooth movement darkens the view's edges while moving; snap turning turns in
-- 45 degree steps, smooth turning at the chosen speed; the menu button leaves VR. MIT License.
local settings = require("game.settings")
local menu = require("ui.menu")

local HEAD = { pos = { 0, 1.65, 0 } }

local function frame(t, left_stick, right_stick, buttons)
  t.xr_frame{ head = HEAD, left = { stick = left_stick or { 0, 0 }, buttons = buttons },
              right = { stick = right_stick or { 0, 0 } } }
end

local function yaw() return (prts.commander.view()) end

local function turned(from, to) -- signed angle in radians, wrapped
  local d = (to - from + math.pi) % (2 * math.pi) - math.pi
  return d
end

return {
  name = "XR comfort defaults, vignette, snap and smooth turning",
  max_seconds = 30,
  run = function(t)
    t.place(0, 0, 30, 0, 0)
    t.ticks(5)
    t.expect(prts.settings.get("xr"), "a first run: nothing saved").equals(nil)
    t.xr_begin()
    frame(t)
    t.ticks(2)
    local x = prts.xr.settings()
    t.expect(x.snap, "no vertex snapping in XR").equals(false)
    t.expect(x.dither, "no dithering in XR").equals(false)
    t.expect(x.vignette, "the comfort vignette is on").is_true()
    t.expect(x.locomotion, "smooth movement").equals("smooth")
    t.expect(x.turn, "snap turning").equals("snap")
    t.expect(prts.render.settings().snap, "the flat screen keeps its vertex snapping").is_true()
    -- the vignette eases in while walking and out after stopping
    frame(t, { 0, -1 })
    t.ticks(15)
    t.expect(prts.xr.state().vignette, "vignette while moving").near(0.6, 1e-3)
    frame(t)
    t.ticks(15)
    t.expect(prts.xr.state().vignette, "gone after stopping").near(0, 1e-3)
    -- snap turning: one 45 degree step per push of the right stick
    local y0 = yaw()
    frame(t, nil, { 1, 0 })
    t.ticks(10)
    t.expect(turned(y0, yaw()), "one snap to the right").near(-math.pi / 4, 1e-3)
    frame(t, nil, { 0.5, 0 })
    t.ticks(2)
    frame(t, nil, { 1, 0 })
    t.ticks(2)
    t.expect(turned(y0, yaw()), "no second snap without centring the stick").near(-math.pi / 4, 1e-3)
    frame(t)
    t.ticks(1)
    frame(t, nil, { -1, 0 })
    t.ticks(2)
    t.expect(turned(y0, yaw()), "a snap back to the left").near(0, 1e-3)
    -- smooth turning at 90 degrees per second, with the vignette
    settings.set_xr("turn", "smooth", false)
    settings.set_xr("turn_speed", 90, false)
    frame(t)
    t.ticks(1)
    y0 = yaw()
    frame(t, nil, { 1, 0 })
    t.ticks(30)
    t.expect(turned(y0, yaw()), "a quarter turn in a second").near(-math.pi / 2, 0.05)
    t.expect(prts.xr.state().vignette > 0.5, "vignette while turning").is_true()
    -- without the comfort vignette
    settings.set_xr("vignette", false, false)
    frame(t, { 0, -1 })
    t.ticks(15)
    t.expect(prts.xr.state().vignette, "no vignette when turned off").near(0, 1e-3)
    -- tracking lost: no views, the commander stays
    local cx, _, cz = t.pos(t.commander())
    t.xr_frame{ lost = true }
    t.ticks(5)
    t.expect(prts.xr.state().tracking, "tracking lost").equals(false)
    local lx, _, lz = t.pos(t.commander())
    t.expect(math.abs(lx - cx) + math.abs(lz - cz), "the body stays").near(0, 1e-3)
    -- the menu button (left Y) leaves VR and opens the pause menu on the flat page
    frame(t, nil, nil, { 0, 0, 0, 0, 0, 1 })
    t.ticks(2)
    t.expect(prts.xr.state().active, "left VR").equals(false)
    t.expect(menu.screen, "the pause menu").equals("pause")
    menu.open(nil)
    for _, key in ipairs({ "turn", "turn_speed", "vignette" }) do settings.set_xr(key, nil, false) end
  end,
}
