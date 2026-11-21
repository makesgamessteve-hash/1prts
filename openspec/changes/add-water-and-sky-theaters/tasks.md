# Tasks: Sail and Wing (M5)

## 1. Water

- [x] 1.1 Engine: `prts.nav.block_below` / `unblock_below` with a C unit test and docs
- [x] 1.2 Sea level, water surface, wading; land units keep to land; verify `wading.lua` (spec *Wading at the shore*)

## 2. Ships

- [x] 2.1 Ship definitions (cog, caravel, war galleon, longship, ram galley), hulls and deck plans, sailing; verify `ships.lua`
- [x] 2.2 Embark and land; verify `ships.lua` (spec *A cog lands troops*)
- [x] 2.3 Broadsides and rams; verify `naval_battle.lua` (spec *A broadside*)
- [x] 2.4 Damage control: rooms, flooding, pumps, sinking; verify `damage_control.lua` (spec *Holed below the waterline*, *The pumps keep her afloat*)
- [x] 2.5 Harbor and raft-yard; the chart room's war table; verify `harbor.lua` (spec *A ship from the harbor*, *Orders from the chart room*)

## 3. Sky

- [x] 3.1 The observation balloon; verify `balloon.lua` (spec *Up in the basket*)
- [x] 3.2 Wyvern riders (sprite, flight, targeting); verify `wyverns.lua` (spec *Wyverns against a balloon*)
- [x] 3.3 The dwarven skyship; verify `skyship.lua` (spec *Over the hills by skyship*)

## 4. The Harbor Siege

- [x] 4.1 Saltcoast: the bay, the town, the siege camp, the fleets; victory and defeat; verify `harbor_siege.lua` (spec *Breaking the siege*, *The galleon lost*)
- [x] 4.2 Docs (README, modding modules, `prts.nav.block_below`), screenshots (`docs/screenshots/60-65`), coverage gates (C 99%, Lua 99.2%), CI green
