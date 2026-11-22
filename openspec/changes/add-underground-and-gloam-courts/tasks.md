# Tasks: Hollow Roads (M7)

## 1. Engine

- [x] 1.1 Terrain holes (`holes`, `prts.scene.hole`, `prts.scene.hole_at`) and the underground rule in collision and navigation, with C tests (`collide.terrain_holes`, `collide.under_the_terrain`, `nav.ignores_galleries_under_the_field`) and docs

## 2. Tunnels

- [x] 2.1 The tunnel kit: galleries, mouths, caverns, cells and portals; verify `tunnels_walk.lua` (spec *Down through a hole*, *Walking a gallery under the field*, *Into the dark*)
- [x] 2.2 Tunnel roads; verify `tunnel_road.lua` (spec *Through the tunnel road*)
- [x] 2.3 Digging; verify `tunnel_dig.lua` (spec *Delvers dig a tunnel*)
- [x] 2.4 Collapse and flooding; verify `tunnel_collapse.lua` (spec *A gallery collapses*, *A flooded gallery*)
- [x] 2.5 Deep mine shafts and echo pillars; verify `deep_shaft.lua`, `echo_pillar.lua` (spec *Down the deep shaft*, *Orders under the ground*)

## 3. The Gloam Courts

- [x] 3.1 Faction, resources, units, buildings, sprites; verify `gloam_economy.lua`, `gloam_units.lua` (spec *The Gloam economy*, *Gloam soldiers*)
- [x] 3.2 Delve, Hollow Court, Sunken Throne with the war room underground; verify `gloam_hq.lua` (spec *Down to the war room*)
- [x] 3.3 The tunnel wyrm; verify `tunnel_wyrm.lua` (spec *The wyrm bursts up*)
- [x] 3.4 Spells; verify `gloam_spells.lua` (spec *Glimmer-wright spells*, *Mycel keeper spells*)

## 4. Matches and scenario

- [x] 4.1 Four-way matches and Hollowfell; verify `fourway_match.lua` (spec *Four sides, three factions*)
- [x] 4.2 Underhill; verify `under_the_hill.lua`, `deep_shaft.lua` (spec *Taking the Hollow Court*, *Caldrith's chain*)
- [x] 4.3 Docs (README, modding modules, engine calls), screenshots (`docs/screenshots/80-85`), coverage gates, CI green
