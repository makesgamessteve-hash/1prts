# 1stPRTS: notes for coding agents

- Read `docs/concept/README.md` and `docs/architecture/README.md` before making design decisions.
- Work is spec-driven with **OpenSpec** (`openspec/`). New features start as a change in
  `openspec/changes/<id>/` (proposal → specs → design → tasks). Implement by working through
  `tasks.md` and checking boxes as tasks are verified. Run `npx openspec validate --strict`
  before committing spec changes. The OpenSpec skills are in `.claude/skills/openspec-*`.
- Hard rules (also in `openspec/config.yaml`):
  - Gameplay is first person only (the First-Person Contract). No overhead, free or third-person
    cameras outside developer builds and tests.
  - Engine in C11 + SDL3 + OpenGL 3.3 core / WebGL 2. Game logic in Lua 5.4 under `data/`.
    No game content compiled into the executable.
  - Must work on Windows, Linux, macOS, web, mobile web (touch) and WebXR.
  - ≥ 95% line coverage for engine C and base-game Lua. New player-facing behavior needs an
    automated game test.
  - Code is MIT. Art is CC BY 4.0 with a `CREDITS.md` entry for every asset file.
  - The simulation is deterministic per build: fixed tick, seeded RNG, no wall-clock time in
    simulation code.
- C symbols use the `prts_` prefix. Only `engine/src/platform/` and `engine/src/xr/` may include SDL or
  Emscripten headers. Only `engine/src/gfx/` may call OpenGL.
