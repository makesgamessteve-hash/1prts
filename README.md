# Banner & Bastion

***War through a commander's eyes.*** Banner & Bastion (codename **1stPRTS**) is a first-person
fantasy real-time strategy game. In the tradition of classic fantasy RTS, serfs haul gold and
timber, knights charge, giants hurl boulders, galleons trade broadsides and drakes burn the sky.
You see all of it only through your commander's eyes, and you lead the way a commander would:
from your fortresses, with banners, war horns, enchanted sending stones, messengers and
face-to-face orders. Keeps, mines, war galleons, siege towers, skyships, walking beast-fortresses
and floating citadels are built at real size with real crews, and you can walk inside every one
of them. The look is retro 3D: low-poly models, sprites and voxels.

> **Status: M1 done; M2 (First Command) to M5 (Sail and Wing) playable.** The C engine runs on Windows, Linux, macOS
> and in the browser (WebGL 2, touch controls on phones, WebXR). It loads all content from
> `data/` and plays two maps in first person. **Harrowmark**, the Crown's border castle, is the
> story map ([the story](docs/concept/15-story.md)). **Ashford Vale** is the first skirmish. Serfs
> mine gold and fell timber, and you build farmsteads, barracks and watchtowers. You train
> men-at-arms, crossbowmen and outriders, and lead them against Lord Varric's host, an AI
> commander. You lead from your own eyes:
> - the war table in your holdfast's war room shows what your side has seen;
> - the field map (Tab) slows you to a walk;
> - the sending stone (Q) carries orders to companies within reach of your holdfast and towers;
> - the marking staff (F) sets a mark;
> - the war horn (H) calls every company within earshot.
>
> **M3 (Ride Out)** adds a proving map, **Greyfen March**:
> - mount a saddled horse (R) and gallop with your knights;
> - stand on a supply wagon as it rolls along its road, or take its driver's seat and drive it;
> - set the crew of a siege tower pushing, climb its stairs while it rolls, and lower its
>   bridge onto the enemy's wall-walk;
> - walk the galleries of a gold mine and ride its lift cage.
>
> **M4 (Hold and Harvest)** adds the second faction and the full economy on **Thornmere**, a
> medium map where the Crown faces the **Wildborn Clans**:
> - Kobb workers, Ghorr brutes, spearhurlers, firebrands, wolf and boar riders, ridge giants;
> - the **behemoth**, a war lodge on a beast's back that walks and that you can climb;
> - aether from springs, Crown powder and Wildborn bone;
> - headquarters that grow through three tiers, and mandates beyond them;
> - workshop upgrades, buildings that rise in stages, timber that burns and is put out;
> - the commander's presence, and succession when the commander falls.
>
> **M5 (Sail and Wing)** takes the war onto the water and into the sky:
> - water, which the commander wades and soldiers keep out of;
> - ships you walk while they sail: the cog, caravel and war galleon, the longship and ram
>   galley. They fight with broadsides and rams, take hits in their rooms, flood and are
>   pumped out or sink. Cogs land troops, and harbors launch ships;
> - the observation balloon, wyvern riders and the dwarven skyship;
> - the scenario **The Harbor Siege** on the Saltcoast: you start in the galleon's chart room
>   at its war table, with a relief fleet to break the Wildborn siege of a harbor town.
>
> The tasks of [`add-command-core`](openspec/changes/add-command-core/tasks.md) (M2),
> [`add-mounts-and-engines`](openspec/changes/add-mounts-and-engines/tasks.md) (M3) and
> [`add-economy-and-second-faction`](openspec/changes/add-economy-and-second-faction/tasks.md)
> (M4) and [`add-water-and-sky-theaters`](openspec/changes/add-water-and-sky-theaters/tasks.md)
> (M5) list what is done and how each part was verified.

![Harrowmark at dusk](docs/screenshots/01_approach_at_dusk.png)

| | | |
|---|---|---|
| ![Harrowmark from the road](docs/screenshots/02_harrowmark_from_afar.png) | ![The King's Gate](docs/screenshots/22_kings_gate.png) | ![The Outer Ward and the Lion Gate](docs/screenshots/18_outer_ward.png) |
| ![On the wall-walk](docs/screenshots/19_wall_walk.png) | ![From the Raven Tower](docs/screenshots/20_raven_tower_view.png) | ![The Inner Ward and the Great Keep](docs/screenshots/21_inner_ward.png) |
| ![The prologue](docs/screenshots/23_prologue.png) | ![The castellan](docs/screenshots/25_the_castellan.png) | ![The war table](docs/screenshots/08_war_table_illusion.png) |
| ![A title card: the Inner Ward](docs/screenshots/26_inner_ward_card.png) | ![One of the castellan's orders done](docs/screenshots/27_order_done.png) | ![The guard at the keep door](docs/screenshots/10_door_guard.png) |
| ![The war table in a skirmish](docs/screenshots/30_war_table_skirmish.png) | ![Serfs at the gold mine](docs/screenshots/31_serfs_at_the_mine.png) | ![The field map](docs/screenshots/32_field_map.png) |
| ![A battle in Ashford Vale](docs/screenshots/33_battle_in_the_vale.png) | ![Lord Varric's holdfast](docs/screenshots/34_varrics_holdfast.png) | ![Saddled horses at the stables](docs/screenshots/40_saddled_horses.png) |
| ![Riding with the knights](docs/screenshots/41_riding_with_the_knights.png) | ![On the wagon](docs/screenshots/42_on_the_wagon.png) | ![Inside the siege tower](docs/screenshots/43_inside_the_siege_tower.png) |
| ![The bridge onto the wall](docs/screenshots/44_bridge_to_the_wall.png) | ![The gold mine's gallery and lift cage](docs/screenshots/45_mine_gallery_and_cage.png) | ![A Wildborn camp](docs/screenshots/50_wildborn_camp.png) |
| ![A behemoth](docs/screenshots/51_behemoth.png) | ![On the behemoth's back](docs/screenshots/52_behemoth_back.png) | ![Fire in the camp](docs/screenshots/53_fire_in_the_camp.png) |
| ![Building in stages](docs/screenshots/54_building_in_stages.png) | ![The galleon's chart room](docs/screenshots/60_chart_room.png) | ![On the galleon's deck](docs/screenshots/61_galleon_deck.png) |
| ![The gun deck](docs/screenshots/62_gun_deck.png) | ![From the balloon's basket](docs/screenshots/64_balloon_view.png) | ![The skyship at its mast](docs/screenshots/65_skyship_at_mast.png) |
| ![Great hall](docs/screenshots/12_hall_benches_braziers.png) | ![Forge](docs/screenshots/05_forge.png) | ![Front-end menu](docs/screenshots/13_front_end_menu.png) |

## Read this first

| | |
|---|---|
| 🎯 **Concept** | [`docs/concept/`](docs/concept/README.md): pitch, pillars, the commander, command and control, scale, units, economy, setting, art, controls, modding, roadmap, open questions, story |
| 🏗️ **Architecture** | [`docs/architecture/`](docs/architecture/README.md): engine and script split, simulation, rendering, data and Lua API, quality and build |
| 📐 **Specs** | [`openspec/`](openspec/): spec-driven development with [OpenSpec](https://github.com/Fission-AI/OpenSpec). Changes: [`add-engine-foundation`](openspec/changes/archive/2026-11-21-add-engine-foundation/proposal.md) (M1), [`add-harrowmark-and-story`](openspec/changes/archive/2026-11-21-add-harrowmark-and-story/proposal.md) (the castle, the story and the title) and [`improve-feel-and-goals`](openspec/changes/archive/2026-11-21-improve-feel-and-goals/proposal.md) (what the fun audit found: movement feel, feedback, goals, a blocked stair, load time) |
| 🎲 **Fun audit** | [`tools/fun/`](tools/fun/README.md): bots with human limits play Harrowmark and the six pillars of the game-fun-evaluator skill are scored. [Results](docs/fun-audit/README.md): 51.5 → 80.5 / 100 after the fixes it led to |
| ❓ **Open questions** | [`docs/concept/13-open-questions.md`](docs/concept/13-open-questions.md) |

## Key facts

- **Engine**: C11, SDL3, OpenGL 3.3 core / WebGL 2. No game content compiled in (`engine/`, `apps/`).
- **Game**: Lua 5.4 scripts plus assets in `data/`, next to the executable. Fully moddable.
- **Design scope**: four playable factions, 84 unit types and 58 building types (see the concept).
- **Platforms**: Windows, Linux, macOS, web (Emscripten), mobile web with automatic touch
  controls, and WebXR.
- **Quality**: ≥ 95% unit-test coverage (C and Lua), automated headless game tests, determinism
  and replay checks, render golden images, browser end-to-end tests, CI on every platform.
- **First-Person Contract**: gameplay never leaves the commander's eyes. See
  [docs/concept/01](docs/concept/01-vision-and-pillars.md#the-first-person-contract).

## Working with specs

```sh
npm install                 # installs the pinned OpenSpec CLI (dev tool only)
npx openspec list           # changes in progress
npx openspec show commander-locomotion --type spec
npx openspec validate --strict
```

## Building and running

Requirements: CMake ≥ 3.24, Ninja, a C11 compiler (GCC, Clang or MSVC) and Git. SDL 3 is
downloaded and built automatically. On Linux you also need the X11/Wayland, GL and audio
development packages SDL 3 lists (`libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev
libxss-dev libxfixes-dev libxtst-dev libxkbcommon-dev libegl-dev libgl-dev`).

```sh
cmake --preset dev            # or: release
cmake --build --preset dev
./build/dev/bin/1stprts       # data/ is copied next to the executable
```

The game opens with the front-end menu; starting Harrowmark there plays a short prologue (it
can be turned off under Options, Game). `--start proving_ground` goes straight to the map,
without it. Controls: WASD to move, mouse to look, Shift to run, Alt to sprint, Ctrl to crouch,
Space to jump, E to use (doors, lift levers, talking to people), Esc for the menu (options,
credits, quit); a gamepad works
too, and every binding can be changed in Options. In developer builds the backquote key opens
the developer console. `1stprts --help` lists the command-line options.

### Tests

```sh
cmake --preset coverage && cmake --build --preset coverage
xvfb-run -a ctest --preset coverage                          # C, Lua, scenario and rendered tests
gcovr --filter engine/src/ --object-directory build/coverage --fail-under-line 95
python3 -I tools/coverage/lua_gate.py build/coverage/lua-coverage --min 95
python3 -I tools/check_layering.py && python3 -I tools/validate_assets.py
```

Automated game tests are Lua scenarios in [`tests/game/scenarios/`](tests/game/scenarios/).
They run headless (`1stprts --headless --scenario <file>`), or with a window to save
screenshots:

```sh
xvfb-run -a ./build/dev/bin/1stprts --size 1920x1080 --scenario tests/game/scenarios/tour.lua --out shots
```

### Web build (Emscripten) and browser tests

Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
(CI uses 7.0.0) and activate it, so `EMSDK` is set. The build produces `1stprts.html`, `.js`,
`.wasm` and the game data as a separate `1stprts.data` (+ `1stprts.data.js`) in
`build/web-release/bin/`; serve that folder over HTTP and open `1stprts.html` (it opens the
front-end menu; `1stprts.html?start=proving_ground` goes straight to the map, like `--start`).

```sh
source "$EMSDK/emsdk_env.sh"
cmake --preset web-release -DPRTS_WEB_TEST_HOOKS=ON   # hooks only for tests; web-dev has them on
cmake --build --preset web-release
python3 -m http.server 8000 --directory build/web-release/bin   # http://localhost:8000/1stprts.html
(cd web && npm ci && npx playwright install chromium) && node web/tests/run.js
```

The browser tests (`web/tests/`) cover boot, keyboard input, touch emulation, the missing-WebGL 2
message, settings kept in IndexedDB across reloads and a WebXR session with a mocked headset.
Real phones and headsets are checked by hand: [`docs/testing/`](docs/testing/).

## License

- **Code** (C, Lua, GLSL, tools): [MIT](LICENSE)
- **Art and content** (models, textures, sprites, voxels, sounds, music, maps…):
  [CC BY 4.0](LICENSE-ASSETS)
- Details, the dividing rule and the attribution format: [LICENSING.md](LICENSING.md)
