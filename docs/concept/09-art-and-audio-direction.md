# 09 · Art & Audio Direction

## 1. Visual target

**"Late-1990s 3D accelerator era, with a modern frame rate."** Low-poly models, small textures
with nearest-neighbor filtering, vertex lighting, distance fog, billboard sprites, a limited color
palette with optional dithering, and chunky bitmap UI on in-world CRTs. The style is our own. We
do not copy any existing game's assets or look.

Everything is drawn at a **low internal resolution** and scaled up with sharp pixels:

| Preset | Internal resolution (16:9) | Use |
|---|---|---|
| Authentic | 320 × 180 | Strongest retro look |
| Retro (default) | 480 × 270 | Default on desktop and web |
| Crisp | 640 × 360 | Mobile and large screens |
| HD | 960 × 540 or native | Accessibility, VR |

### Retro effects (each can be toggled)
| Effect | Default (flat screen) | Default (VR) |
|---|---|---|
| Nearest-neighbor texture filtering | On | On |
| Vertex snapping ("wobble") | Subtle | **Off** (causes discomfort) |
| Affine texture warping | Off | Off |
| Palette quantization + ordered dither | On (per-theater palette) | Off |
| Vertex (Gouraud) lighting with baked vertex color | On | On |
| Distance fog | On | On |
| CRT shader on in-world screens | On | On |
| Scanlines on the whole screen | Off | Off |

## 2. Asset budgets (guidelines)

| Asset | Triangles | Textures |
|---|---|---|
| Small prop | 20–150 | 32² or 64² from a shared atlas |
| Vehicle exterior | 300–1,500 | 128² – 256² atlas |
| Vehicle interior | 300–2,000 | Shared interior kit atlas |
| Aircraft | 400–2,000 | 256² atlas |
| Destroyer exterior | 3,000–8,000 | Modular hull kit |
| Carrier / cruiser / station exterior | 8,000–25,000 | Modular hull kit and LODs |
| Modular interior kit piece | 10–200 | Kit atlas (256² – 512²) |
| Terrain | Chunked height grid, 4 m spacing | Tiled 64² texture set and a splat map |

- **Level of detail**: 2–3 levels per large model, plus an **impostor sprite** for far
  distances. This keeps 16 km sightlines cheap.
- **Palette**: each theater (temperate, desert, arctic, orbit) has a 256-color palette. Faction
  colors are reserved ramps within it.

## 3. Sprites

People, explosions, smoke, fire, trees and debris are **billboard sprites**.
- **People**: 8 view directions. Animation sets: idle, walk, run, crouch, prone, climb, sit,
  work (console), carry, fire, hit, fall. The reference height is 64 px for a 1.8 m person.
- Rank, role and faction are readable from the sprite: helmet shape, color bands, insignia.
- **Effects**: sprite-sheet animations with additive or alpha-tested blending.

## 4. Voxels (optional, used where they add something)

- **Custodian machines** are voxel models, clearly alien next to human low-poly machines.
- **Modder-friendly props**: voxel models (`.vox` files) are converted to meshes when loaded.
  It is the easiest way for non-modelers to add content.
- **Deformable terrain** (later, optional): voxel patches for craters, trenches and tunnels.

## 5. Lighting and sky

- Directional sun (Halvorsen), plus bounce light from **Orun**, plus point lights baked into
  vertex color in interiors. A few dynamic lights for muzzle flashes, explosions and alarm
  beacons.
- Skies are gradient domes with a sprite cloud layer. Orun is a large banded sphere in the sky.
  Space has a star field.
- Interiors have **lighting states**: normal (white or sodium), battle stations (red), power
  loss (emergency strips), vacuum (flashing amber).

## 6. UI and typography

- All gameplay UI is **diegetic**: tables, slates, monitors, signs and stencils.
- Bitmap fonts with a few sizes. Directorate displays are amber, Compact displays are green.
- The helmet HUD uses thin vector lines and a small bitmap font, and keeps to the edges of the
  view.

## 7. Audio

- **Retro tone**: sounds can be authored at 22.05 kHz or 44.1 kHz. A per-sound "crunch" setting
  gives a lo-fi feel without making sounds hard to make out.
- **Positional 3D sound** with distance falloff. **Interior muffling**: outside sound is filtered
  through hulls and walls, and each interior has its own ambience (engine hum, ventilation,
  hull creaks, klaxons).
- **The radio** is a key part of the game. Voice goes through a band-pass filter with noise and
  static, and gets worse with jamming and weak links. Radio lines are assembled from phrase
  pieces (*callsign + verb + object + direction*), so messages can be generated and are easy
  to mod or localize. There are always subtitles.
- **Music**: a synth and tracker-inspired soundtrack that adapts to the situation (calm,
  tension, combat, crisis). Shipboard and station tracks change with the alert state.
- **Formats**: WAV and OGG Vorbis. Tracker modules are a possible later addition.

## 8. Licensing of art

All art is released under **CC BY 4.0** and credited per asset in each pack's `CREDITS.md` (see
[LICENSING.md](../../LICENSING.md)). Placeholder art made during development follows the same
rule, so there is no unlicensed "temporary" content.
