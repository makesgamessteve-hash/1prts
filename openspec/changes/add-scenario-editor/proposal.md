# Proposal: The map and scenario editor (M10)

## Why

Concept 11 promises a first-person map and scenario editor in the engine, plus text formats, so
that players and modders can make battles without leaving the game. It is part of Release 1.0.

## What Changes

- **Map files:** a plain-text format (`game/mapfile.lua`):
  - one instruction per line;
  - read as data and never run;
  - every error names its line.

  It holds the ground (size, seed, hills, time of day), the commander's start, and objects: trees,
  gold mines, aether springs, Imperium ruins, props, holdfasts (one per side) and companies.
- **The player's files:** `prts.files` (engine) reads, writes, lists and removes plain text
  files by name in the user folder's `files/`. Names are never paths.
- **The editor:**
  - a workshop map (`maps/editor.lua`) shows the map file with stand-ins;
  - the commander walks it with the surveyor's rod (`ui/editor.lua`): pick an item from the
    palette, place it on the ground in view, remove or turn what is in view;
  - a readout shows the file, the item and where it would go;
  - the game menu saves, plays, and changes the map's settings (the ground is rebuilt).
- **Playing a map** (`maps/custom.lua`):
  - the map is built for real;
  - two or more holdfasts make a match (side 1 the player's, the others the AI's), with the
    companies placed and the Custodians at the ruins;
  - the game starts again on it from the editor, and the pause menu leads back to the editor.
- **The front-end menu:** "Map editor" lists the map files to edit or play, and makes new ones.
  The skirmishes and scenarios move to a "Battles" screen to keep the main menu short.

## Impact

- New:
  - `engine/src/app/api_files.c`;
  - `game/mapfile.lua`, `game/mapbuild.lua`, `ui/editor.lua`;
  - `maps/editor.lua`, `maps/custom.lua`.
- Changed: `ui/menu.lua`, `scripts/main.lua`.
- Docs: the map format and `prts.files` in the modding docs.
