# Design: The Custodians (M10)

## The side

- **Joining a match.** `Cu.join(stance, ruins)` adds side 9 (`Cu.SIDE`):
  - faction `custodians`, colour `custodian`, `custodians = true`, no AI commander;
  - `stance` is `"neutral"` or `"hostile"`.
- **Ruins.** Each ruin in `ruins` (`{ x, z }`) gets an Imperium ruin (a broken ring of stones
  with a marker) and wakes a guard there: two wardens, a lancer and a mote, plus a spire if
  hostile.
- **Never ends a match.** `M.check_end` skips sides with `custodians`: they are neither lost nor
  counted among those left.
- **Not a war target.** `S.enemies` leaves them out, so no AI chooses them as its war's foe. Its
  soldiers still fight Custodians they meet.

## Constructs (`rts/custodians.lua` definitions)

| id | role | notes |
|---|---|---|
| `custodian.warden` | melee | hp 240, armour 6 |
| `custodian.lancer` | ranged | aether bolt, range 30 |
| `custodian.mote` | flyer | fast, weak, sees far |
| `custodian.colossus` | siege | hp 1400, ×3 against structures (`weapon.structures`) |
| `custodian.spire` | structure | hp 1600, shoots; reclaims wells within `Cu.RECLAIM` (40 m) |

- **Drawn from models.** A unit definition with `model` (a `.vox` path) and `voxel` (metres per
  voxel) is drawn with `prts.scene.model{ entity = e }` instead of a sprite. Such units have no
  `sprite`, so the match skips their animation change.
- **Spawning in batches.** Adding geometry rebuilds the scene, so Custodians are woken a guard
  at a time.
- **The models.** `tools/gen_models.py` makes the five models from boxes and slabs of pale stone
  with glowing aether seams, in the existing `vox_file` writer.

## Behaviour (`Cu.tick`, every second)

- **The ward.** Each construct has a `post` and a `ward` radius (`Cu.WARD`, 45 m):
  - with no enemy inside its ward, a construct farther than 8 m from its post is sent back;
  - an enemy inside the ward is attacked by `K.think` as usual;
  - the leash only stops chases out of the ward.
- **Hostile raids.** Every `Cu.RAID` (180 s), a hostile side sends its free wardens and
  colossi at the nearest other side's headquarters (an attack-move).
- **Reclaiming wells.**
  - A spire marks every aether-producing structure (`def.aether`) of another side within
    `Cu.RECLAIM` as `reclaimed`, and `B.produce` skips reclaimed ones.
  - The mark goes when no spire stands near.
  - The owner gets a dispatch: "The Custodians have taken the well".
- **Rebuilding ruins.**
  - A ruin with Custodians at it and no enemy unit within 30 m for `Cu.REBUILD` (60 s) gets a
    spire.
  - Rebuilding only stops when the ruin's guard is all gone.

## Waking with citadels

- **Aboard a citadel.** `Cu.wake_aboard(citadel)` spawns a guard in the citadel's courtyard and
  core (frame points from `Ci.world`). They ride along, posted in the citadel's frame (posts
  are kept as frame-local points and resolved each second).
- **The skirmish option.** With Custodians on, the event `citadel_woken` wakes the guardians of
  every other citadel on the map.

## The skirmish option

- **The setting.** `game.custodians` is `"off"`, `"neutral"` or `"hostile"`; it is cycled on the
  Game tab.
- **The maps.** Ashford Vale, Thornmere, Hollowfell and Tidesend pass `custodians` and their
  `RUINS` to `match.start`, which calls `Cu.join` when the setting is not off.
