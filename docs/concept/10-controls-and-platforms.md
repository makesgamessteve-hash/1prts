# 10 · Controls & Platforms

## 1. Target platforms

| Platform | Graphics API | Input | Performance target |
|---|---|---|---|
| Windows 10+ (x64) | OpenGL 3.3 core | Keyboard/mouse, gamepad | 60 fps, M map |
| Linux (x64, arm64) | OpenGL 3.3 core | Keyboard/mouse, gamepad | 60 fps, M map |
| macOS 12+ (arm64, x64) | OpenGL 3.3/4.1 core | Keyboard/mouse, gamepad | 60 fps, M map |
| Web, desktop browser | WebGL 2 | Keyboard/mouse, gamepad | 60 fps, M map |
| Web, mobile browser | WebGL 2 | **Touch** (switches automatically) | 30 fps, S map |
| WebXR (standalone headsets and PC VR in browser) | WebGL 2 + WebXR | XR controllers (later: hands) | Headset refresh rate (72/90 Hz), S map |

All platforms play the same game with the same data. Platforms differ only in presentation and
in defaults (resolution, draw distance, how many crew are fully simulated).

## 2. Action-based controls

The game reads **actions** (e.g. `move`, `look`, `use`, `field_map`, `spyglass`, `stone`,
`horn`, `order_confirm`), not raw keys. Actions are defined in Lua. Bindings are stored in a
user config file and can be remapped in the options menu.

### Keyboard and mouse (default)
| Action | Binding |
|---|---|
| Move / look | WASD / mouse |
| Run / sprint (hold) | Shift |
| Crouch / jump or climb | Ctrl / Space |
| Use, interact, sit, mount, step up to the war table | E |
| Field map (toggle) | Tab |
| Spyglass (hold) / zoom | Right mouse / mouse wheel |
| Mark order / mark sighting (with the spyglass or staff) | Left mouse / middle mouse |
| Sending stone menu (hold) | Q |
| Horn signals (hold for menu) | H |
| Select company group 1–9 / assign group | 1–9 / Ctrl + 1–9 |
| Give the signal (for orders waiting on a signal) | G |
| Lantern | F |
| Call your mount | R |
| Menu | Esc |

At the **war table**: the mouse moves a pointer on the map, WASD pans, the wheel zooms, Q and E
rotate, left mouse selects (drag to sweep-select), right mouse gives a context order. The
runes on the table edge pick explicit order types.

### Gamepad
Left stick to move and right stick to look. Triggers handle the spyglass and marking. Bumpers
open the sending stone and field map. Face buttons handle use, jump, crouch and cancel. At the
war table, the right stick moves the pointer.

### Touch (mobile web)
The web build **switches to touch controls on its own** when it detects a touch-first device,
and switches back if a mouse or keyboard is used.
- **Left half of the screen**: floating virtual stick for movement.
- **Right half**: drag to look, tap to use or select.
- **Context buttons** (bottom right): Field map, Spyglass, Stone, Horn, Crouch, Run, Mount. They
  only show when they can be used.
- **Sending stone and horn**: press and hold, then slide to pick from the radial menu.
- **Field map and war table**: tap to select, drag to sweep-select, pinch to zoom, two-finger
  drag to pan, long-press for the context order menu.
- **Marking**: while looking through the spyglass, tap a target, then pick an order from the
  radial menu.
- Touch controls are drawn as a screen overlay. This is the one intended exception to the
  diegetic rule, and the screen overlay is kept to a minimum.

### WebXR
VR is a natural fit for a physical commander.
- **Locomotion**: smooth movement with the thumbstick or teleport (comfort option). Snap or
  smooth turning. Optional comfort vignette. A seated mode. Riding uses smooth movement with a
  stronger vignette.
- **War table**: reach into the illusion. Grab with both hands to pan, rotate or scale. Point
  and pull the trigger to select and give orders.
- **Field map**: held in the off hand like a real map. Use it with the pointer from the other
  hand.
- **Spyglass**: raise the controller to your eye, or press the spyglass button.
- **Marking staff**: point the controller; the beam comes out of the staff in your hand.
- **Ladders, ropes and handholds**: grab with your hands. Weightless movement in the aether core
  is hand over hand.
- **Sending stone and horn**: radial menus on the controller. Raise the horn to your mouth to
  sound it.
- Retro vertex wobble and screen effects are **off** in VR for comfort.

## 3. Accessibility

- Full remapping of keyboard, mouse, gamepad and touch.
- Subtitles for all voices, stone messages and horn signals, with a dispatch log.
- Colorblind-safe sigils (shape and color together) and selectable palettes.
- Any retro effect can be turned off. FOV slider. Head bob, mount sway and camera shake can be
  turned off.
- Hold-to-press actions can be changed to toggles.
- Scalable UI on the field map and war table, plus a high-contrast table mode.
- Game speed options (single player) and a command pause rule.
