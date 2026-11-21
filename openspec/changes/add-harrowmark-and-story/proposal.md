# Proposal: Harrowmark, a story and a title ("Banner & Bastion")

## Why

The engine foundation (M1) ends with a test map whose keep is the only building. The concept
promises realistically huge fortresses that can be walked through, and a war told through the
commander's eyes, but the game had neither a real castle nor a story, and its working title
("1stPRTS") said nothing to players. Building one great castle now proves that the engine,
the brushes and the collision scale up to M2's fortresses, and it gives the story a place.

## What Changes

- The game is called **Banner & Bastion** ("War through a commander's eyes"). "1stPRTS" stays
  the codename for the repository, executable, file names and C prefix.
- The Proving Ground becomes **Harrowmark**, a concentric castle around the existing keep: an
  inner curtain (15 m) with six towers, the 36 m Raven Tower and the Lion Gate; an outer curtain
  (10 m) with fifteen towers and the King's Gate; wall-walks with battlements, stairs up the walls,
  towers that can be entered at ground and wall-walk height, working portcullises, halls in both
  wards and a garrison.
- A written story (`docs/concept/15-story.md`) told in first person in the game: a prologue in
  subtitles when a map is started from the menu, people who talk when used, and written orders on
  the hall's notice board.
- Engine additions: hollow round walls (towers) as a scene shape, boxes that only the use ray
  hits, a separate script budget for data files loaded by a running script, and the window title
  read from the base pack's manifest.

Engine (C): the tube shape, use-only boxes, the data-file script budget, the window title.
Scripts and data (Lua, assets): the castle building blocks (`lib/castle.lua`), the portcullis,
the map, the subtitles, dialogue, prologue, options and all text.

## Impact

- Windows, Linux, macOS, web desktop: the larger map loads and runs; CI runs the castle and story
  scenarios headless on every desktop platform and the smoke set in the browser.
- Mobile web: talking uses the same `use` action (the touch overlay's use button and taps).
- WebXR: talking and the portcullis winch work with the controller pointer (use ray).
- Specs: new capabilities `castle-content` and `storytelling`.

## Out of scope

- Siege, damage and capture of the castle (M3/M4), moats and drawbridges, tunnels.
- Voice acting and lip sync; branching dialogue; quests.
- Renaming the repository, the executable or the code prefix.
