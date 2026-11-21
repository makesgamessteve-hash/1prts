# Proposal: Ride Out (M3), mounts and engines

## Why

M2 made a real-time strategy game of the commander's eyes, but the commander still walks
everywhere and every place they can stand stays still. The concept's promise is to lead from the
saddle, ride inside the war machines and walk their decks while they move (concept 02, 04, 05).
M3 lets you ride with your knights and climb a siege tower while it is being pushed, up to the
enemy's wall. You can also ride a cage down into the galleries of a gold mine.

## What Changes

- **The commander on horseback:**
  - Horses stand saddled at the stables.
  - Mounting raises the eye to the saddle and lets the commander trot, canter and gallop; dismounting leaves the horse where it stands.
  - A horse's head and neck are in view while riding.
- **Cavalry:** stables train knights (heavy, a charge that hits hardest at speed) and outriders. A
  "Ride with me" order on the sending stone and a horn signal make a company follow the
  commander.
- **Moving frames (engine):** the commander already rides what they stand on. M3 adds a view
  that turns with a turning frame. It also uses moving frames for real:
  - wagons on a road;
  - a siege tower pushed to a wall;
  - a mine's lift cage.
- **Posts:** seats and stations the commander takes and leaves with use:
  - the wagon's driver's seat (the move keys drive the wagon);
  - the siege tower's push bar (a word to the crew to push or stop);
  - the tower's bridge lever.
- **Deck plans:** a compiler (`lib/deckplan.lua`) that turns a plan written as text rows per
  level into floors, walls, doorways, windows and stairs, and into rooms (zones) with collision.
  It works inside a moving frame. The siege tower and the gold mine's galleries are deck plans.
- **Crews and levels of detail:** a crew is a roster of people in rooms of a frame.
  - Near the commander they are embodied (sprites at their stations, moving with the frame).
  - Farther away they are rostered (names and rooms, no sprites).
  - Far away they are aggregated (a count).
- **Room damage, first version:** rooms of a frame take damage. People in a room that is hit can
  fall, and a wrecked room shows it.
- **A proving map, *Greyfen March*:**
  - the Crown's camp with stables and horses, and a wagon road;
  - a rival fort with a curtain wall and towers;
  - a siege tower in front of the fort;
  - a gold mine in a hill, with an adit, galleries and a lift cage to the upper gallery.

Engine (C): the turning view on a turning frame. Scripts and data: everything else.

## Impact

- Every platform: riding and the posts use existing actions plus one new action, `mount` (R,
  the gamepad's d-pad up, a touch button; in XR, use on the horse).
- Saves: posts, mounts, crews and rooms are entity data (saved and hashed). No format change.
- Tests: a game test for every scenario in the spec deltas; Lua unit tests for the deck-plan
  compiler.
