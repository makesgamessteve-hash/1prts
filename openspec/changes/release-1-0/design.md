# Design: Release 1.0 (M10)

## Mod entry points

- **Where they run.** `scripts/main.lua` loops over `prts.vfs.mounts()` (load order) after
  requiring the base game's modules. For each mod with `scripts/mods/<id>/init.lua` in its own
  pack (`prts.vfs.read_from`), it runs `pcall(require, "mods.<id>.init")`.
- **Why that path.** It holds the mod's id, so no mod's entry point can replace another's.
- **Logging.** Success is logged; an error is logged with the mod's id, and boot carries on.

## The example mod

- **Where it lives.** `examples/mods/watchfire/` is shipped as source, not inside `data/`.
  `engine/tests/CMakeLists.txt` copies `examples/mods` into every scenario's user area, next to
  the test mods.
- **The test.** `tests/game/scenarios/example_mod.lua` runs with `--mod watchfire --start
  watchfire_hill` and checks:
  - the map is offered and plays;
  - two sides;
  - its own unit;
  - the Custodians at its ruin;
  - its rule;
  - its language and translations.

## Version

1.0.0 in `CMakeLists.txt` (`project`), `PRTS_ENGINE_VERSION` and `data/base/mod.lua`. Mods
depending on `base >= 0.1` still load.
