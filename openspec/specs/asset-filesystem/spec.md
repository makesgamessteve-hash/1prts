# asset-filesystem Specification

## Purpose

Loads every game asset, script and shader from folders next to the executable through a virtual file system that supports mods, safe paths, graceful failure and correct CC BY 4.0 attribution.

## Requirements

### Requirement: Content lives outside the executable
The executable SHALL NOT contain game content. Scripts, shaders, models, textures, sprites, voxels, sounds, music, fonts and maps SHALL be loaded at run time from the `data/` folder next to the executable, or from the separate data package on the web.

#### Scenario: Changing an asset without rebuilding
- **WHEN** a texture file in `data/base/textures/` is replaced with another valid image and the game is restarted
- **THEN** the game shows the new texture without the executable being rebuilt

#### Scenario: Data folder missing
- **WHEN** the executable is started and no `data/` folder can be found
- **THEN** a built-in error screen explains that the data folder is missing and where it was searched for, and the game exits with a non-zero status

### Requirement: Mod mounting and overrides
The file system SHALL mount the base game first and then each enabled mod in load order. When several mounts contain the same virtual path, the file from the last mount SHALL be used. Directory listings SHALL merge all mounts.

#### Scenario: Mod overrides a base file
- **WHEN** an enabled mod contains `sounds/door_open.wav` and the base game also contains it
- **THEN** opening a door plays the mod's sound

#### Scenario: Mod adds a new file
- **WHEN** an enabled mod contains `scripts/defs/props/new_crate.lua` that the base game does not have
- **THEN** listing `scripts/defs/props/` returns the base files and the mod's new file

#### Scenario: Incompatible mod
- **WHEN** a mod's manifest declares a scripting API major version different from the engine's
- **THEN** the mod is not mounted, a clear message names the mod and both versions, and the game continues without it

### Requirement: Safe virtual paths
Virtual paths SHALL be relative, use `/` as separator and be case-sensitive on every platform. Paths that are absolute, contain `..`, backslashes, drive letters or empty segments SHALL be rejected. Game content SHALL be read-only, and writing SHALL be possible only in the user data area.

#### Scenario: Path escape attempt
- **WHEN** a script asks to read `../../etc/passwd` or `C:/Windows/win.ini`
- **THEN** the request fails with an invalid-path error and no file outside the mounts is accessed

#### Scenario: Same behavior across platforms
- **WHEN** a script reads `Models/Box.glb` but the file is stored as `models/box.glb`
- **THEN** the read fails on Windows, macOS, Linux and web alike

### Requirement: User data area
Settings, key bindings, saves, replays and logs SHALL be stored in a per-user data area separate from the install folder. On the web this area SHALL persist between visits in the same browser.

#### Scenario: Settings persist
- **WHEN** the player changes the retro resolution preset, quits and starts the game again
- **THEN** the chosen preset is still active

### Requirement: Missing or broken assets
A missing or unreadable asset SHALL be replaced by a clearly visible placeholder (or a short placeholder sound) and SHALL be logged with its path. It SHALL NOT crash the game.

#### Scenario: Missing model
- **WHEN** a map references `models/missing.glb` which does not exist
- **THEN** the map loads, a placeholder object is shown where the model should be, and the log names the missing path

### Requirement: Asset attribution
Every asset pack SHALL include a `CREDITS.md` that names title, author, license, source and changes for every art and content file. The game SHALL show a Credits screen built from the credits of all mounted packs and the third-party code licenses.

#### Scenario: Credits screen
- **WHEN** the player opens the Credits screen from the front-end menu
- **THEN** the screen lists the credits entries of the base game and of every enabled mod

#### Scenario: Uncredited asset detected
- **WHEN** the asset validation check runs and a file under `data/` is not covered by any `CREDITS.md` entry
- **THEN** the check fails and names the uncovered file
