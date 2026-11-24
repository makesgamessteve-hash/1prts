# Proposal: Release 1.0, modding guide and mod entry points (M10)

## Why

Release 1.0 promises modding documentation and an API reference (concept 11, 12). So far a mod
could only replace files, or ask to have its scripts required by a test. It had no supported way
to add a map, a unit or a language at boot. 1.0 gives mods an entry point, documents it in a
guide with a working example, and sets the game's version to 1.0.0.

## What Changes

- **Mod entry points:** the base game's boot runs every enabled mod's
  `scripts/mods/<id>/init.lua` in load order, after its own modules and before the first map. A
  failing one is logged and skipped.
- **The modding guide** (`docs/modding/guide.md):
  - a mod's folder and manifest;
  - the entry point;
  - maps, units and rules;
  - languages;
  - the editor's map format;
  - credits and licenses;
  - testing.

  The API reference gains the M10 modules (Custodians, campaign, editor, localization) and
  `prts.files`.
- **The example mod**, Watchfire Hill (`examples/mods/watchfire/`): a map on the Battles screen,
  a new unit, a rule change, a French language and German for its own words. A scenario test
  runs it in CI.
- **Version 1.0.0** for the engine (`PRTS_ENGINE_VERSION`), the project and the base game's
  manifest.

## Impact

- Changed: `scripts/main.lua`, `CMakeLists.txt`, `prts_base.h`, `data/base/mod.lua`,
  `engine/tests/CMakeLists.txt` (installs the example mod for scenarios).
- New: `docs/modding/guide.md`, `examples/mods/watchfire/`, `tests/game/scenarios/example_mod.lua`.
