# Tests

Where the automated tests live and how to run them. The test layers, coverage gates and CI jobs
are described in [`docs/architecture/quality-and-build.md`](../docs/architecture/quality-and-build.md).

| Folder | What | Run by |
|---|---|---|
| `engine/tests/` | C unit tests (utest.h), one or more suites per engine module | CTest `prts_unit_tests` (`build/<preset>/bin/prts_tests`) |
| `data/base/scripts/tests/` | Lua unit tests of the base game (`T.case`) | CTest `lua_unit_tests` (`1stprts --headless --test-lua`) |
| `tests/game/scenarios/` | Automated game tests: headless play sessions driven through the action layer | CTest `scenario_<name>` (`1stprts --headless --scenario <file>`) |
| `tests/game/determinism/` | Walks that are recorded, replayed (every tick's state hash must match) and recorded again (byte-identical) | CTest `determinism_<name>_record`, `_replay`, `_rerun`, `_identical` |
| `tests/golden/` | Render golden images: fixed views compared with `tests/golden/ref/` (8 levels, at most 0.5% of pixels); refresh with `xvfb-run -a tools/golden/update` | CTest `render_golden`, `render_golden_catches_fog_change` (need a display) |
| `web/tests/` | Browser end-to-end tests of the web build (Playwright + Chromium): boot, keyboard, audio unlock, mobile emulation with touch, IndexedDB, XR mock, and the scenario smoke set run in the page | `node web/tests/run.js` (CI `web` job; needs a web-release build with `-DPRTS_WEB_TEST_HOOKS=ON`) |
| `tools/tests/` | Self-tests of the repository checks (asset validator, API documentation and spec coverage checks) | `python3 -I tools/tests/test_validate_assets.py` and `test_check_scripts.py` in the CI `checks` job |

```sh
cmake --preset dev && cmake --build --preset dev
ctest --preset dev                      # render_tour needs a display (CI uses xvfb-run)
build/dev/bin/prts_tests --filter='replay.*'
ctest --preset dev -L smoke            # the commander-is-there smoke set (walk, stairs, door, lift, war table)
build/dev/bin/1stprts --headless --scenario tests/game/determinism/keep_walk.lua --record walk.rpl
build/dev/bin/1stprts --headless --replay walk.rpl --junit replay.xml
```

Every Lua test, scenario and replay run writes a JUnit XML report to `build/<preset>/junit/`.

Scenarios with a `-- ctest-labels: smoke` line form the commander-is-there smoke set. Each names the
cell the commander must end in (`final_cell`); the runner reports every result with the final cell and
the state hash, and `web/tests/smoke.test.js` runs the same files in the browser
(`1stprts.html?test=1&scenario=<name>`), where they must pass with the same final cell.

## Spec scenarios and their tests

Every scenario in the OpenSpec specs (`openspec/changes/*/specs/**/spec.md` and
`openspec/specs/**/spec.md`) has a row here. `tools/check_spec_coverage.py` (CI `checks` job)
fails when a scenario has no row, a row names a scenario that does not exist, or a referenced test
is missing. References are C unit tests (`suite.case`), test files and scripts (paths), or
`pending (task X.Y)` for scenarios whose test arrives with that task.

| Capability | Scenario | Covered by |
|---|---|---|
| asset-filesystem | Changing an asset without rebuilding | `vfs.mod_overrides_and_case` (content is read from the data folder at run time), `gfx.windowed_engine_renders_and_recovers_from_broken_content` (a mod's files replace the base ones without a rebuild); a texture-swap restart test is pending (task 7.3) |
| asset-filesystem | Data folder missing | `engine.boot_errors` |
| asset-filesystem | Mod overrides a base file | `vfs.mod_overrides_and_case`; `data/base/scripts/tests/test_audio.lua` (every door and lift sound path loads) |
| asset-filesystem | Mod adds a new file | `vfs.merged_listing` |
| asset-filesystem | Incompatible mod | `engine.mods_are_checked_and_mounted`, `mods.dependencies_and_load_order` |
| asset-filesystem | Path escape attempt | `vfs.path_rules`, `script.vfs_api` |
| asset-filesystem | Same behavior across platforms | `vfs.mod_overrides_and_case` |
| asset-filesystem | Settings persist | `vfs.user_area` (storage); pending (task 12.4) |
| asset-filesystem | Missing model | `model.missing_and_broken_models_show_a_placeholder`, `engine.game_api_from_scripts` (`prts.scene.model` with a missing path) |
| asset-filesystem | Credits screen | pending (task 12.2) |
| asset-filesystem | Uncredited asset detected | `tools/tests/test_validate_assets.py` |
| audio-playback | Sound to the left | `audio.sound_to_the_left`, `tests/game/scenarios/audio_keep.lua` |
| audio-playback | Cross-fade | `audio.cross_fade_between_streams` |
| audio-playback | Muting music | `audio.muting_music`, `tests/game/scenarios/audio_keep.lua` |
| audio-playback | Door closes on a noisy room | `audio.muffling_takes_high_frequencies_away`, `tests/game/scenarios/audio_keep.lua` (the forge door opens and closes) |
| audio-playback | First click starts music | `web/tests/boot.test.js` (before the first click the browser's audio is suspended, nothing is mixed and the menu music waits at its start; after the click it plays), `audio.start_requests_wait_until_the_device_runs` |
| automated-testing | Headless on a server | `engine.headless_ticks_and_determinism`, `tests/game/scenarios/walk_speed.lua` |
| automated-testing | Failing scenario | `engine.scenario_outcomes`, `engine/tests/fixtures/scenarios/fails.lua`, `engine.scenario_result_with_final_cell_and_hash` (a scenario's `final_cell`: failed with the expected and actual cell), `engine/tests/fixtures/scenarios/wrong_cell.lua` |
| automated-testing | Report for CI | `report.runners_write_junit`, `report.junit_xml_escaping_and_cases` |
| automated-testing | Running base game tests | `engine.lua_test_runner_reports_failures`, `data/base/scripts/tests/test_props.lua` |
| automated-testing | Coverage drops | `.github/workflows/ci.yml` (gcovr `--fail-under-line 95`), `tools/coverage/lua_gate.py` |
| automated-testing | Lua coverage report | `script.lua_line_coverage`, `tools/coverage/lua_gate.py` |
| automated-testing | Non-deterministic code is caught | `replay.nondeterministic_script_is_caught_at_the_first_differing_tick` |
| automated-testing | Visual regression | `tests/golden/golden.lua` (CTest `render_golden`), `tests/golden/fog_changed.lua` (CTest `render_golden_catches_fog_change`) |
| automated-testing | Emulated phone | `web/tests/mobile.test.js` (Pixel 7 profile: the touch controls are drawn and a right-half drag turns the view) |
| automated-testing | Windows-only failure | `.github/workflows/ci.yml` (windows job); pending (task 1.5) |
| commander-locomotion | Release build | pending (task 9.6) |
| commander-locomotion | Developer observer camera | pending (task 9.6) |
| commander-locomotion | Walking speed | `tests/game/scenarios/walk_speed.lua`, `commander.walking_speed_and_crouch` |
| commander-locomotion | Crouching lowers the view | `commander.walking_speed_and_crouch` |
| commander-locomotion | Stairs | `tests/game/scenarios/spiral_stair.lua` (the keep's spiral stair to the roof), `tests/game/scenarios/keep_stairs.lua` |
| commander-locomotion | Wall at sprint speed | `tests/game/scenarios/sprint_into_wall.lua`, `collide.sliding_stepping_and_ground`, `collide.slope_limit_on_terrain` |
| commander-locomotion | Opening a door | `tests/game/scenarios/keep_door.lua` |
| commander-locomotion | Riding the keep's hoist lift | `tests/game/scenarios/keep_lift.lua`, `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Walking during movement | `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Reaching the war room | `tests/game/scenarios/keep_lift.lua`, `tests/game/scenarios/war_table.lua`, `tests/game/determinism/keep_walk.lua` |
| input-actions | Mod adds an action | `engine.game_api_from_scripts`, `data/base/scripts/tests/test_input.lua` |
| input-actions | Rebinding use | pending (task 8.3) |
| input-actions | Gamepad hot-plug | `platform.events_become_device_state` |
| input-actions | Phone browser | `web/tests/mobile.test.js` (an emulated phone starts in the touch scheme with the overlay drawn), `engine.status_for_test_hooks_and_scheme_hint` |
| input-actions | Tablet with a keyboard | `tests/game/scenarios/touch_controls.lua`, `web/tests/mobile.test.js` (a key switches an emulated phone to keyboard and mouse) |
| input-actions | Touch movement and look | `touch.move_and_look_at_the_same_time`, `tests/game/scenarios/touch_controls.lua`, `web/tests/mobile.test.js` (browser touch events on both halves at once) |
| input-actions | Prompt update | `input.schemes_and_overrides`; scheme events to scripts are pending (task 8.4) |
| lua-scripting | Replacing the boot script | `engine.mods_are_checked_and_mounted` |
| lua-scripting | Stale entity handle | `data/base/scripts/tests/test_input.lua` |
| lua-scripting | Blocked operating system access | `script.sandbox_blocks_os_access` |
| lua-scripting | Module loading through the file system | `script.require_through_vfs_with_overrides` |
| lua-scripting | Error in an event handler | `script.error_containment_in_event_handlers` |
| lua-scripting | Endless loop | `script.budget_interrupts_endless_loop` |
| lua-scripting | Same seed, same random numbers | `script.deterministic_random_and_time` |
| lua-scripting | Changing a door script while playing | `reload.changing_a_door_script_while_playing`, `reload.modules_are_rerun_in_place_and_on_reload_runs` |
| lua-scripting | Running a command | pending (task 12.3) |
| platform-targets | Build on each desktop platform | `.github/workflows/ci.yml` (linux, windows and macos jobs); pending (task 1.5) |
| platform-targets | OpenGL context unavailable | `platform.impossible_gl_version_is_reported` |
| platform-targets | Web bundle runs in a browser | `web/tests/boot.test.js` |
| platform-targets | Browser without WebGL 2 | `web/tests/no_webgl2.test.js` |
| platform-targets | Same scenario result on desktop and web | `web/tests/smoke.test.js` (the smoke set, `tests/game/scenarios/keep_lift.lua` among it, run in the browser must pass with the `final_cell` the desktop CTest run ends in: CTest `scenario_keep_lift`, label `smoke`), `engine.scenario_result_with_final_cell_and_hash` |
| platform-targets | Extract and run | pending (task 14.1) |
| platform-targets | Licenses included | pending (task 14.1) |
| retro-rendering | Changing the preset | `gfx.windowed_engine_renders_and_recovers_from_broken_content`, `tests/golden/golden.lua` (approach_authentic, approach_retro, approach_crisp) |
| retro-rendering | Turning off dithering | `tests/golden/golden.lua` (hall_default, hall_no_dither, hall_affine, hall_no_snap) |
| retro-rendering | Mod changes the look | `gfx.windowed_engine_renders_and_recovers_from_broken_content` |
| retro-rendering | Broken shader | `gfx.windowed_engine_renders_and_recovers_from_broken_content` |
| retro-rendering | Directional sprite | `tests/golden/golden.lua` (guard_front, guard_side, guard_back) |
| retro-rendering | Voxel prop | `model.vox_greedy_meshing_and_placement`, `model.base_game_models_load` |
| retro-rendering | Dusk lighting | `tests/golden/golden.lua` (approach_retro: the Proving Ground's dusk preset) |
| retro-rendering | Field map and horizon in one view | `tests/golden/golden.lua` (field_map_and_horizon) |
| retro-rendering | Closed door hides a room | `cells.closed_door_hides_a_room`, `tests/golden/golden.lua` (forge_door_closed, forge_door_open, with drawn-cell checks) |
| retro-rendering | Live war table | `gfx.windowed_engine_renders_and_recovers_from_broken_content`, `tests/golden/golden.lua` (war_table) |
| retro-rendering | Two views | `gfx.windowed_engine_renders_and_recovers_from_broken_content` (stereo views and a WebXR frame), `xr.eye_views_follow_the_commander` |
| simulation-core | Different frame rates, same simulation | `clock.fixed_ticks_independent_of_frame_rate` |
| simulation-core | Very slow frame | `clock.slow_frame_is_capped` |
| simulation-core | High refresh display | `world.interpolation_between_ticks` |
| simulation-core | Two identical runs | `engine.headless_ticks_and_determinism`, `replay.record_and_replay_match_every_tick`, `tests/game/determinism/keep_walk.lua` (CTest `determinism_keep_walk_identical`) |
| simulation-core | Divergence is detected | `world.state_hash_detects_divergence` |
| simulation-core | Record and replay | `tests/game/determinism/keep_walk.lua` (CTest `determinism_keep_walk_replay`), `tests/game/determinism/xr_walk.lua` (XR head and hand input, CTest `determinism_xr_walk_replay`), `replay.record_and_replay_match_every_tick` |
| simulation-core | Slot reuse | `world.spawn_names_and_slot_reuse` |
| simulation-core | Riding a moving frame | `world.nested_frames_and_reparenting` |
| simulation-core | Changing parent without a jump | `world.nested_frames_and_reparenting` |
| simulation-core | Far from the origin | `core.double_precision_far_from_origin` |
| xr-presentation | Entering VR | `web/tests/xr.test.js`, `xr.eye_views_follow_the_commander` |
| xr-presentation | No XR support | `web/tests/boot.test.js` (no Enter VR control without support) |
| xr-presentation | Session ended by the headset | `web/tests/xr.test.js` |
| xr-presentation | Leaning toward a wall | `tests/game/scenarios/xr_lean_wall.lua` (the view stops short of the keep's wall; the body follows the head; seated mode), `commander.xr_body_follows_the_head_and_walls_push_the_view_back` |
| xr-presentation | Pulling the lift lever | `tests/game/scenarios/xr_lift_lever.lua`, `xr.recorded_frames_pull_the_lift_lever` (a recorded pose fixture, `engine/tests/fixtures/xr/lever_pull.txt`), `xr.controllers_drive_actions_by_hand`, `commander.xr_pointer_ray_uses_things_but_not_through_walls`, `tests/game/determinism/xr_walk.lua` |
| xr-presentation | Teleport movement | `tests/game/scenarios/xr_teleport.lua`, `commander.xr_teleport_to_reachable_floor_only`, `web/tests/xr.test.js` (the setting saved in the user area, the mocked headset's stick and ray) |
| xr-presentation | Comfort defaults | `tests/game/scenarios/xr_comfort.lua` (also the vignette, snap and smooth turning, the menu button), `web/tests/xr.test.js`, `data/base/scripts/tests/test_xr.lua` |
| castle-content | Approaching the castle | `tests/golden/golden.lua` (approach views of Harrowmark from the road), `tests/game/scenarios/castle_gate.lua` (walks in through the King's Gate), `tests/game/scenarios/walk_speed.lua` (starts on the road) |
| castle-content | Onto the wall-walk and into a tower | `tests/game/scenarios/castle_walls.lua`, `data/base/scripts/tests/test_castle.lua` |
| castle-content | To the top of the great tower | `tests/game/scenarios/raven_tower.lua`, `shapes.towers_face_out_and_in_with_open_sides` |
| castle-content | Closing the gate | `tests/game/scenarios/castle_gate.lua`, `data/base/scripts/tests/test_castle.lua` (portcullis states) |
| storytelling | Front-end menu | `tests/game/scenarios/front_end_menu.lua`, `data/base/scripts/tests/test_story.lua` (title, map name) |
| storytelling | Starting Harrowmark from the menu | `tests/game/scenarios/story_prologue.lua`, `data/base/scripts/tests/test_story.lua` (prologue only from the menu, option) |
| storytelling | Talking to a guard | `tests/game/scenarios/npc_talk.lua`, `data/base/scripts/tests/test_story.lua`, `collide.use_boxes_stop_only_the_use_ray` |
| storytelling | Reading the board | `tests/golden/golden.lua` (notice_board: the orders drawn on the board's surface), `data/base/scripts/tests/test_story.lua` (the board's text fits) |
