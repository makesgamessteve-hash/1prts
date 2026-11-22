# Tasks: Every Screen (M9)

## 1. Engine

- [x] 1.1 `prts.render.settings{ draw_distance }`, `prts.render.stats()`; checked by the windowed gfx test in `test_engine.c` (2 m draw distance) and `performance.lua`
- [x] 1.2 XR riding vignette (`ride_vignette`, `ride_strength`, `carried`); checked by `xr_ride_comfort.lua`
- [x] 1.3 `prts.input.toggle`; C test `input.toggled_actions_latch`
- [x] 1.4 `prts.time.scale`; C test `engine.game_speed_scales_ticks`

## 2. Touch and hands

- [x] 2.1 Touch scale, left-handed layout, context labels; verify `touch_polish.lua` (spec *Left-handed with large buttons*, *Buttons under the water*)
- [x] 2.2 WebXR hands in the web glue; verify `web/tests/xr.test.js` (spec *Pinching to walk and to use*)

## 3. Comfort and performance

- [x] 3.1 Riding vignette option; verify `xr_ride_comfort.lua` (spec *Riding a ship in VR*)
- [x] 3.2 Draw distance and profiles; verify `performance.lua`, C test (spec *A shorter draw distance*, *A phone starts on low*)

## 4. Accessibility

- [x] 4.1 Palettes, sigils, high contrast, map text; verify `accessibility_maps.lua` (spec *A colour-blind palette*, *Larger map text*)
- [x] 4.2 Toggles, brightness boost, game speed, options menu; verify `accessibility_play.lua` (spec *Toggle crouch*, *Brightness under the ground*, *Faster game*)
- [x] 4.3 Docs, screenshots (`docs/screenshots/96-99`), coverage gates, CI green
