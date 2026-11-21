# 09 · Art & Audio Direction

## 1. Visual target

**"Late-1990s 3D fantasy, with a modern frame rate."** Low-poly castles and ships, small
hand-painted textures with nearest-neighbor filtering, vertex lighting, warm torchlight, distance
fog over the valleys, billboard sprites for people and creatures, a limited color palette with
optional dithering, and parchment and runic UI drawn on objects in the world. The style is our
own. We do not copy any existing game's assets or look.

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
| Palette quantization + ordered dither | On (palette per region) | Off |
| Vertex (Gouraud) lighting with baked vertex color | On | On |
| Distance fog | On | On |
| Magic-surface shader on in-world displays (war table, scrying pools) | On | On |
| Underwater tint and caustics | On | On (softer) |
| Scanlines on the whole screen | Off | Off |

## 2. Asset budgets (guidelines)

| Asset | Triangles | Textures |
|---|---|---|
| Small prop (barrel, cart, crate) | 20–150 | 32² or 64² from a shared atlas |
| Siege engine, wagon | 300–1,500 | 128² – 256² atlas |
| Creature, large (giant, boar, wyvern) | 300–1,500, rigid parts | 128² – 256² atlas |
| Small building | 200–1,000 | Building kit atlas |
| Galleon / bonehulk exterior | 3,000–8,000 | Modular hull kit |
| Castle / behemoth / floating citadel exterior | 8,000–25,000 | Modular kit and LODs |
| Modular interior kit piece | 10–200 | Kit atlas (256² – 512²) |
| Terrain | Chunked height grid, 4 m spacing | Tiled 64² texture set and a splat map |
| Trees | Crossed-sprite or low-poly cards, many instances | Foliage atlas |
| Tunnel and cavern kit piece | 20–400 | Rock and fungus atlas |
| Reef and kelp props | Cards and low-poly instances | Reef atlas |

- **Level of detail**: 2–3 levels per large model, plus an **impostor sprite** for far
  distances. This keeps 16 km views over valleys cheap.
- **Palette**: each region type (green valleys, marsh, winter highlands, coast and reef, under
  water, the deep caverns, the storm layer, the high sky) has a 256-color palette. Faction colors are reserved ramps within it.

## 3. Sprites, creatures and animation

People and person-sized creatures are **billboard sprites**.
- **People** (humans, dwarves, Ghorr, Kobb, Durrow, Hesk) and **mounted units**: 8 view directions. Animation
  sets: idle, walk, run, crouch, climb, sit, work (mine, chop, hammer), carry (sack, log), attack,
  shoot or throw, cast, hit, fall, and for riders also gallop and rear. Swimmers also have swim,
  dive and surface; Durrow also have dig.
- Role, rank and faction are readable from the sprite: helmet or headdress shape, color, banner.
- **Large creatures** (ridge giants, wyverns, rocs, elder drakes, the behemoth, the tunnel wyrm,
  olm brutes, giant beetles, lantern moths, albatrosses, mantas, reef crabs) are **low-poly
  models made of rigid parts** (head, neck, body, legs, wings, segments), animated part by part the way
  late-1990s games did it. No skinning is needed, and the look stays chunky and retro.
- **Effects**: fire, smoke, magic and dust are sprite-sheet animations with additive or
  alpha-tested blending.
- **Trees** are cheap cards or low-poly instances. A felled tree becomes a log prop and a stump.

## 4. Voxels (optional, used where they add something)

- **Custodian constructs** are voxel models, so they look clearly different from the people and
  beasts of Veyra.
- **Modder-friendly props**: voxel models (`.vox` files) are converted to meshes when loaded. It
  is the easiest way for non-modelers to add content.
- **Deformable ground** (later, optional): voxel patches for siege trenches, tunnels dug at run
  time (Gloam tunnel roads, sapper tunnels) and collapsing galleries.

## 5. Lighting and sky

- A directional sun, plus soft fill from the sky, plus point lights baked into vertex color in
  interiors (torches, hearths, lanterns, forges). A few dynamic lights for fire, spells, muzzle
  flashes and lightning.
- Skies are gradient domes with a sprite cloud layer. The two moons, Ilen (pale) and Corvane
  (red), and the dark shapes of drifting citadels are big landmarks in the sky.
- **Underground**, the only light is glowstone (blue-violet), torches and lanterns. Deep caverns
  are lit in pools, with darkness between them.
- **Under water**, light fades and turns blue-green with depth. Shafts of light and caustic
  sprites play over the reef near the surface.
- In the **storm layer**, everything is grey cloud lit by lightning flashes.
- Interiors have **lighting states**: day (shafts of light through windows), night (torchlight),
  alarm (lanterns lit, bells), on fire (orange flicker and smoke).

## 6. UI and typography

- All gameplay UI is **diegetic**: the war table's illusion map, the field map's ink, dispatches,
  shop signs, banners and painted marks.
- Bitmap fonts in a few sizes. The Crown's displays are golden lines, the Wildborn's green spirit
  fire, the Gloam's blue-violet glowstone, the Brine League's turquoise.
- The HUD uses thin lines and a small bitmap font, and keeps to the edges of the view.

## 7. Audio

- **Retro tone**: sounds can be authored at 22.05 kHz or 44.1 kHz. A per-sound "crunch" setting
  gives a lo-fi feel without making sounds hard to make out.
- **Positional 3D sound** with distance falloff. **Interior muffling**: sound from outside is
  filtered through walls and hulls, and each interior has its own ambience (crackling hearths,
  creaking timbers, dripping mine water, the behemoth's heartbeat). Tunnels and caverns echo.
  Under water, everything is muffled and low, with the creak of hulls overhead.
- **Unit voices** are a big part of the genre's charm. Acknowledgements ("Aye, my lord!",
  "Hrah!", "Below, and onward.", "Aye aye, Admiral!") and reports are built from phrase pieces (*name + verb + object + direction*), so
  messages can be generated and are easy to mod or localize. There are always subtitles.
- **Sending-stone voices** go through a whispery filter with a little echo and crackle, and get
  worse near hush wards and on weak links.
- **Horns, drums, bells and thunder** are gameplay sounds: horn signals, the Wildborn's war drums
  carrying across valleys, a castle's alarm bell, the hum of a Gloam echo pillar, the thunder of a
  Weatherwright's storm.
- **Music**: a synth and tracker-inspired fantasy soundtrack (pipes, drums, choir pads, lutes)
  that adapts to the situation (calm, tension, battle, siege). Each of the four factions has its own themes (strings and brass for the Crown, drums and
  horns for the Wildborn, low choirs and chimes for the Gloam, shanties and fiddles for the Brine
  League).
- **Formats**: WAV and OGG Vorbis. Tracker modules are a possible later addition.

## 8. Licensing of art

All art is released under **CC BY 4.0** and credited per asset in each pack's `CREDITS.md` (see
[LICENSING.md](../../LICENSING.md)). Placeholder art made during development follows the same
rule, so there is no unlicensed "temporary" content.
