# Lua 5.4.7 — vendored copy

- Source: https://github.com/lua/lua, tag `v5.4.7` (commit `1ab3208a1fceb12fca8f24ba57d6e13c5bff15e3`).
- Only the core VM and the libraries 1stPRTS opens are kept. Removed: `lua.c`, `onelua.c`,
  `ltests.*`, `liolib.c`, `loslib.c`, `loadlib.c`, `ldblib.c`, `linit.c` (the engine opens its own
  sandboxed set of libraries).
- License: MIT (see `LICENSE`).

## Patches

1. `src/luaconf.h`: defines `luai_makeseed(L)` as a constant so string hashing, and with it the
   iteration order of `pairs()`, is the same on every run of the same build. Upstream seeds it
   from the clock and memory addresses. This is needed for deterministic simulation and replays.
