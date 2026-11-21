# Third-party code

| Library | Version | License | Used for |
|---|---|---|---|
| [Lua](https://www.lua.org) | 5.4.7 (`lua/lua` tag `v5.4.7`) | MIT | Game scripting (see `lua/PATCHES.md`) |
| [stb_image, stb_image_write](https://github.com/nothings/stb) | commit `6b5799bae8a1` | MIT or public domain | PNG loading and screenshots |
| [cgltf](https://github.com/jkuhlmann/cgltf) | commit `85cd62382dfe` | MIT | glTF 2.0 model loading |
| [utest.h](https://github.com/sheredom/utest.h) | commit `fc4715da3b3e` | Unlicense | C unit tests (test builds only) |
| [SDL 3](https://libsdl.org) | `release-3.4.18` (fetched by CMake) | zlib | Platform layer |

Third-party code is built with warnings off and is not counted in coverage.
