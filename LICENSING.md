# Licensing

1stPRTS uses two licenses: one for code and one for art.

| What | License | File |
|---|---|---|
| **Code**: C engine sources, build scripts, tools, tests, Lua scripts, shader sources | MIT | [`LICENSE`](LICENSE) |
| **Art and content**: models, textures, sprites, voxel models, sounds, music, fonts, maps, deck plans, written content in `data/` | Creative Commons Attribution 4.0 International (CC BY 4.0) | [`LICENSE-ASSETS`](LICENSE-ASSETS) |
| **Third-party code** in `third_party/` | The license that ships with each library | `third_party/<lib>/LICENSE*` |

## Where the line falls

The `data/` folder holds both code and art, so one rule decides which license applies:

- Files under any `scripts/` folder (`*.lua`) and shader sources (`*.glsl`, `*.vert`, `*.frag`) are **code** and use **MIT**.
- Every other file under `data/` is **art/content** and uses **CC BY 4.0**, unless its asset pack's
  `CREDITS.md` says otherwise. An asset under another license is allowed only if that license is CC BY 4.0-compatible
  (CC0 or CC BY), and its credits entry must name that license.

## Attribution

Each asset pack (for example `data/base/`) contains a `CREDITS.md` listing every asset with:

```
<path-glob> | <title> | <author(s)> | <license> | <source URL or "original"> | <changes made>
```

The in-game **Credits** screen is built from these files, and an automated test fails if any
file under `data/` has no credits entry. This keeps CC BY 4.0 attribution correct as mods and
contributions grow.

To credit 1stPRTS art when you reuse it:

> "<asset title>" by <author> from 1stPRTS (https://github.com/makesgamessteve-hash/1prts), licensed under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Changes: <describe or "none">.

## Contributions

By contributing you agree that your code is released under MIT and your art/content under
CC BY 4.0, and that you have the right to release it that way.
