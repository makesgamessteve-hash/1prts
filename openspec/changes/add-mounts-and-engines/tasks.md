# Tasks: Ride Out (M3)

## 1. Engine

- [x] 1.1 The view turns with a turning parent frame (not in XR); verify `commander.the_view_turns_with_a_turning_frame` and `wagon_ride.lua` (spec *Standing on a wagon*)

## 2. Mounts and cavalry

- [x] 2.1 Horse and mounted sprites (horse, knight, mounted outrider; crown and red); credits
- [x] 2.2 Horses, mounting and dismounting, the horse's head in view, the `mount` action; verify `ride_horse.lua` (spec *Mounting and galloping*, *Dismounting*)
- [x] 2.3 Stables, knights, the charge; verify `knights_charge.lua` (spec *Knights charge*)
- [x] 2.4 "Ride with me" on the stone and the horn; verify `ride_with_knights.lua` (spec *Ride with your knights*)

## 3. Frames, posts, deck plans, crews

- [x] 3.1 Deck-plan compiler; verify `scripts/tests/test_deckplan.lua` (spec *A small plan*)
- [x] 3.2 Posts; the wagon with its road and driver's seat; verify `wagon_ride.lua` (spec *Standing on a wagon*, *Driving the wagon*)
- [x] 3.3 The siege tower (deck plan, push bar, bridge); verify `siege_tower.lua` (spec *Up the tower to the wall*)
- [x] 3.4 Crews with levels of detail; verify `crew_lod.lua` (spec *Walking away from the siege tower*)
- [x] 3.5 Room damage; verify `room_damage.lua` (spec *The tower's push room is wrecked*)
- [x] 3.6 The gold mine (adit, galleries, lift cage); verify `mine_galleries.lua` (spec *Walking the galleries of a gold mine*)

## 4. Map, docs, CI

- [x] 4.1 Greyfen March in the menu
- [x] 4.2 Docs (README status, modding API and base-game modules), screenshots (`docs/screenshots/40-45`), coverage gates (C 99%, Lua 99.5%), CI green
