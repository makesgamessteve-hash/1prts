# Tasks: Skyfall (M6)

## 1. Engine

- [x] 1.1 `prts.scene.aim_surface` and fog-only `prts.scene.env` without a scene rebuild, with C tests and docs

## 2. Citadels

- [x] 2.1 The citadel frame, deck plan, track drift; verify `citadel_track.lua` (spec *A citadel drifts on its track*)
- [x] 2.2 Garrison, docking skyships, waking at the aetherstone; verify `citadel_wake.lua` (spec *Waking the citadel*, *No caster, no waking*)
- [x] 2.3 The aether core; verify `aether_core.lua` (spec *Floating in the aether core*)
- [x] 2.4 Steering, sight and the scrying floor; verify `citadel_steer.lua` (spec *Steering the citadel*, *Seeing from the citadel*)
- [x] 2.5 Drop-gates and bombardment; verify `citadel_drop.lua` (spec *Through the drop-gates*, *Bombarding from the sky*)

## 3. The storm layer

- [x] 3.1 Cloud deck, sight, fog, winds, lightning; verify `storm_layer.lua` (spec *Inside the storm*, *Carried by the wind*, *Struck by lightning*)

## 4. Magic

- [x] 4.1 Spells, Magister and Bone-seer, Arcanum and Spirit lodge; verify `spells.lua` (spec *Firebolt*, *No aether, no spell*, *Wither*)
- [x] 4.2 Far-sight; verify `far_sight.lua` (spec *Far-sight*)

## 5. Tier 6

- [x] 5.1 Sky mast, sky galleon, elder drake, boarding; verify `tier_six.lua` (spec *Only at tier six*, *The sky galleon's guns*, *Drakes board a citadel*)

## 6. Storm the Citadel

- [x] 6.1 Caldmere, Caldrith rising, Ashby, boarders, victory and defeat; verify `storm_the_citadel.lua`, `storm_the_citadel_lost.lua` (spec *Storming Caldrith*, *Caldrith broken*)
- [x] 6.2 A citadel over Thornmere; verify `citadel_skirmish.lua` (spec *A citadel over Thornmere*)
- [x] 6.3 Docs (README, modding modules, engine calls), screenshots (`docs/screenshots/70-75`), coverage gates, CI green
