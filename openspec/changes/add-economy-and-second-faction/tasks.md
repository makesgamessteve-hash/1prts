# Tasks: Hold and Harvest (M4)

## 1. Factions and the economy

- [x] 1.1 Faction roles; the rules, AI, tools and war table use them instead of Crown names
- [x] 1.2 Aether springs, wells and shrines; powder at the Forgeworks; bone from herd pens and remains; verify `econ_resources.lua` (spec *An aether well on a spring*, *Powder at the Forgeworks*, *Kobb gather bone from the fallen*)
- [x] 1.3 Tiers (HQ stages), tier requirements and mandates; verify `econ_tiers.lua` (spec *Upgrading the holdfast to a keep*, *A mandate needs its buildings*)
- [x] 1.4 Workshop upgrades; verify `econ_tiers.lua` (spec *Tempered blades*)

## 2. Construction and fire

- [x] 2.1 Construction stages and presence while building; verify `build_stages.lua` (spec *A barracks rises in stages*, *Presence speeds building*)
- [x] 2.2 Fire, spread and firefighting, stone that does not burn; verify `fire.lua` (spec *Firebrands burn a barracks*, *Serfs put the fire out*, *Stone does not burn*)

## 3. Presence and succession

- [x] 3.1 Presence in battle; verify `presence_succession.lua` (spec *Holding with the commander*)
- [x] 3.2 The commander's health, falling, successors, handover, defeat; verify `presence_succession.lua` (spec *The banner passes to the second*, *No one left to lead*)

## 4. The Wildborn

- [x] 4.1 Sprites (Kobb, Ghorr, spearhurler, firebrand, wolf runner, boar rider, ridge giant) and credits
- [x] 4.2 Wildborn definitions, buildings and shapes
- [x] 4.3 The behemoth; verify `behemoth.lua` (spec *A war lodge on the move*)
- [x] 4.4 The AI for both factions, aether, upgrades, expansion; verify `rts_wildborn_match.lua`, `ai_expands.lua` (spec *Crown against Wildborn*, *The AI expands*)

## 5. Map, docs, CI

- [x] 5.1 Thornmere in the menu
- [x] 5.2 Docs (README, modding modules), screenshots (`docs/screenshots/50-54`), coverage gates (C 99%, Lua 99.1%), CI green
