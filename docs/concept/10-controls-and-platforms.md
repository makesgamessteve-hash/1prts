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

The game reads **actions** (e.g. `move`, `look`, `use`, `slate`, `binoculars`, `radio`,
`order_confirm`), not raw keys. Actions are defined in Lua. Bindings are stored in a user
config file and can be remapped in the options menu.

### Keyboard and mouse (default)
| Action | Binding |
|---|---|
| Move / look | WASD / mouse |
| Run / sprint (hold) | Shift |
| Crouch / jump or climb | Ctrl / Space |
| Use, interact, sit, engage table | E |
| Slate (toggle) | Tab |
| Binoculars (hold) / zoom | Right mouse / mouse wheel |
| Designate / mark (with binoculars) | Left mouse / middle mouse |
| Radio menu (hold) | Q |
| Select group 1–9 / assign group | 1–9 / Ctrl + 1–9 |
| Go-code broadcast | G |
| Flashlight / night vision | F / N |
| Menu | Esc |

At a **map table**: the mouse moves a pointer on the map, WASD pans, the wheel zooms, Q and E
rotate, left mouse selects (drag for a box), right mouse gives a context order. Edge buttons
pick explicit order types.

### Gamepad
Left stick to move and right stick to look. Triggers handle binoculars and designate. Bumpers
open the radio and slate. Face buttons handle use, jump, crouch and cancel. At a table, the
right stick moves the pointer.

### Touch (mobile web)
The web build **switches to touch controls on its own** when it detects a touch-first device,
and switches back if a mouse or keyboard is used.
- **Left half of the screen**: floating virtual stick for movement.
- **Right half**: drag to look, tap to use or select.
- **Context buttons** (bottom right): Slate, Binoculars, Radio, Crouch, Run. They only show
  when they can be used.
- **Radio**: press and hold, then slide to pick from the radial menu.
- **Slate and map table**: tap to select, drag for box select, pinch to zoom, two-finger drag
  to pan, long-press for the context order menu.
- **Designator**: while in binoculars, tap a target, then pick an order from the radial menu.
- Touch controls are drawn as a screen overlay. This is the one intended exception to the
  diegetic rule, and the screen overlay is kept to a minimum.

### WebXR
VR is a natural fit for a physical commander.
- **Locomotion**: smooth movement with the thumbstick or teleport (comfort option). Snap or
  smooth turning. Optional comfort vignette. A seated mode.
- **Map table**: reach into the hologram. Grab with both hands to pan, rotate or scale. Point
  and pull the trigger to select and order.
- **Slate**: held in the off hand (or worn on the wrist). Use it with the pointer from the
  other hand.
- **Binoculars**: raise the controller to your eyes, or press the binocular button.
- **Ladders and handholds**: grab with your hands. Zero-g movement is hand over hand.
- **Radio**: a radial menu on the controller.
- Retro vertex wobble and screen effects are **off** in VR for comfort.

## 3. Accessibility

- Full remapping of keyboard, mouse, gamepad and touch.
- Subtitles for all radio and dialogue, with a radio log.
- Colorblind-safe symbols (shape and color together) and selectable palettes.
- Any retro effect can be turned off. FOV slider. Head bob and camera shake can be turned off.
- Hold-to-press actions can be changed to toggles.
- Scalable UI on screens and slate, plus a high-contrast table mode.
- Game speed options (single player) and a command pause rule.
