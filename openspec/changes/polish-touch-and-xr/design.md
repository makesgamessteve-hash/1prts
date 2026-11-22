# Design: Every Screen (M9)

## Touch (`ui/touch.lua`, `game/settings.lua`)

- **The layout.** `T.layout(opts)` builds the touch layout from `T.LAYOUT` with:
  - `scale` (0.75–1.5) multiplying every button's radius;
  - `left_handed`, which mirrors the buttons (x → 1 − x) and puts the stick on the right half
    (`split` mirrored, and `stick_side` passed to the engine as `split` 0.5 with the halves
    swapped).

  The settings `touch.scale` and `touch.left_handed` apply it.
- **Labels follow context:**
  - swimming: jump reads "Rise" and crouch "Dive";
  - on a horse: the Ride button reads "Dismount";
  - a held tool: Map reads "Lower".

## WebXR hands (`web/prts_xr.js`)

- **Asking for it.** `requestSession` asks for `hand-tracking` (optional).
- **Reading a hand.** For an input source with a `hand` and no gamepad, the glue reads the
  `thumb-tip` and `index-finger-tip` joint poses (`frame.getJointPose`). A pinch is a fingertip
  distance under 2.5 cm, released above 4 cm (hysteresis).
- **Packing it.** The hand is packed like a controller:
  - grip pose: the wrist;
  - aim pose: the target ray;
  - button 0 (trigger): the pinch;
  - for the left hand, the stick pushed forward (axis 3 = −1) while it pinches.

  The engine is unchanged. A pinch uses like a trigger, and a left pinch walks, or aims a
  teleport in teleport mode.
- **Testing it.** The mock gains `useHands(true)` and per-hand `pinch`.

## Comfort when carried (`engine/src/app/engine_xr.c`)

- **When it shows.** Each frame the commander's world position is compared with the last frame's.
  When they moved more than 0.5 m/s while the player was not walking, and the option
  `ride_vignette` is on (default on), the vignette's target is `ride_strength` (default 0.8),
  or the normal strength if that is higher.
- **Options.** `prts.xr.settings{ ride_vignette, ride_strength }`; `prts.xr.state().carried`.

## Performance

- **Draw distance.** `prts.render.settings{ draw_distance = m }` (50–5000; default 5000):
  - sprites farther than that from the eye, and the mesh groups of entities whose position is
    farther, are skipped;
  - the fog's far distance is clamped to it.
- **Counters.** `prts.render.stats()` returns `{ sprites, entity_groups }` drawn in the last frame.
- **Profiles.** `game.performance` is `low` (draw distance 350, retro preset), `medium` (900) or
  `high` (5000). On the first boot it is `low` when the input scheme is touch or VR, and `high`
  otherwise.

## Accessibility

- **Palettes.** `ui/war_maps.lua` keeps `W.PALETTES`: `default`, `deuteranopia` and `protanopia`
  (blue against orange), `tritanopia` (red against teal), and `high_contrast` (black and white
  with yellow and magenta). `game.palette` picks one.
- **Sigils.** Your own companies are drawn as rings and the enemy's as squares, in every palette.
- **Map text.** `game.map_text` (1 or 2) scales the war table's and field map's text.
- **Toggles.** `game.toggle_crouch` and `game.toggle_run` call `prts.input.toggle`. A toggled
  action latches on a press and unlatches on the next press; the engine's `prts.input.value` and
  `down` report the latch.
- **Brightness.** `game.dark_boost` lifts the fog layers `underground` and `underwater` (`lib/fog.lua`):
  - their far distance doubles;
  - their colour is lightened halfway to grey.
- **Game speed.** `game.speed` (0.5, 1, 1.5, 2) sets `prts.time.scale`, which scales the real time
  fed to the fixed-tick clock. Scenarios and replays ignore it.
