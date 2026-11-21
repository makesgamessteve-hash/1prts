# Third-party code

| Library | Version | License | Used for |
|---|---|---|---|
| [Lua](https://www.lua.org) | 5.4.7 (`lua/lua` tag `v5.4.7`) | MIT | Game scripting (see `lua/PATCHES.md`) |
| [stb_image, stb_image_write](https://github.com/nothings/stb) | commit `6b5799bae8a1` | MIT or public domain | PNG loading and screenshots |
| [cgltf](https://github.com/jkuhlmann/cgltf) | commit `85cd62382dfe` | MIT | glTF 2.0 model loading |
| [stb_vorbis](https://github.com/nothings/stb) | v1.25, commit `6b5799bae8a1` | MIT or public domain | OGG Vorbis decoding (sounds, streamed music) |
| [microui](https://github.com/rxi/microui) | 2.02 (commit `0850aba86095`) | MIT | Menus, options, credits and the developer console (behind `prts_ui`) |
| [utest.h](https://github.com/sheredom/utest.h) | commit `fc4715da3b3e` | Unlicense | C unit tests (test builds only) |
| [SDL 3](https://libsdl.org) | `release-3.4.18` (fetched by CMake) | zlib | Platform layer |

Third-party code is built with warnings off and is not counted in coverage. microui is used
unmodified; it is built without the sanitizers' alignment check because it packs its draw
commands without padding (`engine/src/ui/ui.c` reads them through aligned copies).
