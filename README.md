# 1stPRTS

**A first-person fantasy real-time strategy game.** In the tradition of classic fantasy RTS,
serfs haul gold and timber, knights charge, giants hurl boulders, galleons trade broadsides and
drakes burn the sky. You see all of it only through your commander's eyes. Keeps, mines, war
galleons, siege towers, skyships, walking beast-fortresses and floating citadels are built at real
size with real crews, and you can walk inside every one of them. Orders travel by enchanted
sending stones, war horns, banners, messengers and face-to-face conversations. The look is retro
3D: low-poly models, sprites and voxels.

> **Status: concept phase (M0).** No code yet. This repository holds the game concept, the
> architecture and the first OpenSpec change (M1 · Proving Ground), ready for review before
> implementation starts.

## Read this first

| | |
|---|---|
| 🎯 **Concept** | [`docs/concept/`](docs/concept/README.md): pitch, pillars, the commander, command and control, scale, units, economy, setting, art, controls, modding, roadmap, open questions |
| 🏗️ **Architecture** | [`docs/architecture/`](docs/architecture/README.md): engine and script split, simulation, rendering, data and Lua API, quality and build |
| 📐 **Specs** | [`openspec/`](openspec/): spec-driven development with [OpenSpec](https://github.com/Fission-AI/OpenSpec). The first change: [`add-engine-foundation`](openspec/changes/add-engine-foundation/proposal.md) |
| ❓ **Open questions** | [`docs/concept/13-open-questions.md`](docs/concept/13-open-questions.md) |

## Key facts

- **Engine**: C11, SDL3, OpenGL 3.3 core / WebGL 2. No game content compiled in.
- **Game**: Lua 5.4 scripts plus assets in `data/`, next to the executable. Fully moddable.
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
npx openspec show add-engine-foundation
npx openspec validate --strict
```

## Building

There is nothing to build yet. Build, run and test instructions arrive with milestone M1
(task 14.4 in [`tasks.md`](openspec/changes/add-engine-foundation/tasks.md)).

## License

- **Code** (C, Lua, GLSL, tools): [MIT](LICENSE)
- **Art and content** (models, textures, sprites, voxels, sounds, music, maps…):
  [CC BY 4.0](LICENSE-ASSETS)
- Details, the dividing rule and the attribution format: [LICENSING.md](LICENSING.md)
