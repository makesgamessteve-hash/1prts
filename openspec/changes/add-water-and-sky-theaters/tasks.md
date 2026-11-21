# Tasks: Sail and Wing (M5)

## 1. Water

- [ ] 1.1 Engine: `prts.nav.block_below` / `unblock_below` with a C unit test and docs
- [ ] 1.2 Sea level, water surface, wading; land units keep to land; verify `wading.lua` (spec *Wading at the shore*)

## 2. Ships

- [ ] 2.1 Ship definitions (cog, caravel, war galleon, longship, ram galley), hulls and deck plans, sailing; verify `ships.lua`
- [ ] 2.2 Embark and land; verify `ships.lua` (spec *A cog lands troops*)
- [ ] 2.3 Broadsides and rams; verify `naval_battle.lua` (spec *A broadside*)
- [ ] 2.4 Damage control: rooms, flooding, pumps, sinking; verify `damage_control.lua` (spec *Holed below the waterline*, *The pumps keep her afloat*)
- [ ] 2.5 Harbor and raft-yard; the chart room's war table; verify `harbor.lua` (spec *A ship from the harbor*, *Orders from the chart room*)

## 3. Sky

- [ ] 3.1 The observation balloon; verify `balloon.lua` (spec *Up in the basket*)
- [ ] 3.2 Wyvern riders (sprite, flight, targeting); verify `wyverns.lua` (spec *Wyverns against a balloon*)
- [ ] 3.3 The dwarven skyship; verify `skyship.lua` (spec *Over the hills by skyship*)

## 4. The Harbor Siege

- [ ] 4.1 Saltcoast: the bay, the town, the siege camp, the fleets; victory and defeat; verify `harbor_siege.lua` (spec *Breaking the siege*, *The galleon lost*)
- [ ] 4.2 Docs, screenshots, coverage gates, CI green
