# Tests

Where the automated tests live and how to run them. The test layers, coverage gates and CI jobs
are described in [`docs/architecture/quality-and-build.md`](../docs/architecture/quality-and-build.md).

| Folder | What | Run by |
|---|---|---|
| `engine/tests/` | C unit tests (utest.h), one or more suites per engine module | CTest `prts_unit_tests` (`build/<preset>/bin/prts_tests`) |
| `data/base/scripts/tests/` | Lua unit tests of the base game (`T.case`) | CTest `lua_unit_tests` (`1stprts --headless --test-lua`) |
| `tests/game/scenarios/` | Automated game tests: headless play sessions driven through the action layer | CTest `scenario_<name>` (`1stprts --headless --scenario <file>`) |
| `tests/game/determinism/` | Walks that are recorded, replayed (every tick's state hash must match) and recorded again (byte-identical) | CTest `determinism_<name>_record`, `_replay`, `_rerun`, `_identical` |
| `tools/tests/` | Self-tests of the repository checks (asset validator, API documentation and spec coverage checks) | `python3 -I tools/tests/test_validate_assets.py` and `test_check_scripts.py` in the CI `checks` job |

```sh
cmake --preset dev && cmake --build --preset dev
ctest --preset dev                      # render_tour needs a display (CI uses xvfb-run)
build/dev/bin/prts_tests --filter='replay.*'
build/dev/bin/1stprts --headless --scenario tests/game/determinism/keep_walk.lua --record walk.rpl
build/dev/bin/1stprts --headless --replay walk.rpl --junit replay.xml
```

Every Lua test, scenario and replay run writes a JUnit XML report to `build/<preset>/junit/`.

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
| asset-filesystem | Mod overrides a base file | `vfs.mod_overrides_and_case`; the door sound itself is pending (task 11.3) |
| asset-filesystem | Mod adds a new file | `vfs.merged_listing` |
| asset-filesystem | Incompatible mod | `engine.mods_are_checked_and_mounted`, `mods.dependencies_and_load_order` |
| asset-filesystem | Path escape attempt | `vfs.path_rules`, `script.vfs_api` |
| asset-filesystem | Same behavior across platforms | `vfs.mod_overrides_and_case` |
| asset-filesystem | Settings persist | `vfs.user_area` (storage); pending (task 12.4) |
| asset-filesystem | Missing model | pending (task 7.3) |
| asset-filesystem | Credits screen | pending (task 12.2) |
| asset-filesystem | Uncredited asset detected | `tools/tests/test_validate_assets.py` |
| audio-playback | Sound to the left | pending (task 11.1) |
| audio-playback | Cross-fade | pending (task 11.3) |
| audio-playback | Muting music | pending (task 11.1) |
| audio-playback | Door closes on a noisy room | pending (task 11.4) |
| audio-playback | First click starts music | pending (task 11.5) |
| automated-testing | Headless on a server | `engine.headless_ticks_and_determinism`, `tests/game/scenarios/walk_speed.lua` |
| automated-testing | Failing scenario | `engine.scenario_outcomes`, `engine/tests/fixtures/scenarios/fails.lua` |
| automated-testing | Report for CI | `report.runners_write_junit`, `report.junit_xml_escaping_and_cases` |
| automated-testing | Running base game tests | `engine.lua_test_runner_reports_failures`, `data/base/scripts/tests/test_props.lua` |
| automated-testing | Coverage drops | `.github/workflows/ci.yml` (gcovr `--fail-under-line 95`), `tools/coverage/lua_gate.py` |
| automated-testing | Lua coverage report | `script.lua_line_coverage`, `tools/coverage/lua_gate.py` |
| automated-testing | Non-deterministic code is caught | `replay.nondeterministic_script_is_caught_at_the_first_differing_tick` |
| automated-testing | Visual regression | pending (task 7.9) |
| automated-testing | Emulated phone | pending (task 13.6) |
| automated-testing | Windows-only failure | `.github/workflows/ci.yml` (windows job); pending (task 1.5) |
| commander-locomotion | Release build | pending (task 9.6) |
| commander-locomotion | Developer observer camera | pending (task 9.6) |
| commander-locomotion | Walking speed | `tests/game/scenarios/walk_speed.lua`, `commander.walking_speed_and_crouch` |
| commander-locomotion | Crouching lowers the view | `commander.walking_speed_and_crouch` |
| commander-locomotion | Stairs | `tests/game/scenarios/keep_stairs.lua` |
| commander-locomotion | Wall at sprint speed | `collide.sliding_stepping_and_ground`; the sprint scenario is pending (task 9.2) |
| commander-locomotion | Opening a door | `tests/game/scenarios/keep_door.lua` |
| commander-locomotion | Riding the keep's hoist lift | `tests/game/scenarios/keep_lift.lua`, `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Walking during movement | `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Reaching the war room | `tests/game/scenarios/keep_lift.lua`, `tests/game/determinism/keep_walk.lua` |
| input-actions | Mod adds an action | `engine.game_api_from_scripts`, `data/base/scripts/tests/test_input.lua` |
| input-actions | Rebinding use | pending (task 8.3) |
| input-actions | Gamepad hot-plug | `platform.events_become_device_state` |
| input-actions | Phone browser | pending (task 8.5) |
| input-actions | Tablet with a keyboard | pending (task 8.4) |
| input-actions | Touch movement and look | pending (task 8.5) |
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
| platform-targets | Web bundle runs in a browser | pending (task 3.4) |
| platform-targets | Browser without WebGL 2 | pending (task 3.4) |
| platform-targets | Same scenario result on desktop and web | pending (task 10.5) |
| platform-targets | Extract and run | pending (task 14.1) |
| platform-targets | Licenses included | pending (task 14.1) |
| retro-rendering | Changing the preset | `gfx.windowed_engine_renders_and_recovers_from_broken_content`, `gfx.settings_and_null_backend`; golden images pending (task 7.4) |
| retro-rendering | Turning off dithering | `gfx.windowed_engine_renders_and_recovers_from_broken_content`; golden images pending (task 7.5) |
| retro-rendering | Mod changes the look | `gfx.windowed_engine_renders_and_recovers_from_broken_content` |
| retro-rendering | Broken shader | `gfx.windowed_engine_renders_and_recovers_from_broken_content` |
| retro-rendering | Directional sprite | pending (task 7.6) |
| retro-rendering | Voxel prop | pending (task 7.3) |
| retro-rendering | Dusk lighting | pending (task 7.5) |
| retro-rendering | Field map and horizon in one view | pending (task 7.4) |
| retro-rendering | Closed door hides a room | pending (task 7.7) |
| retro-rendering | Live war table | pending (task 7.8) |
| retro-rendering | Two views | pending (task 7.4) |
| simulation-core | Different frame rates, same simulation | `clock.fixed_ticks_independent_of_frame_rate` |
| simulation-core | Very slow frame | `clock.slow_frame_is_capped` |
| simulation-core | High refresh display | `world.interpolation_between_ticks` |
| simulation-core | Two identical runs | `engine.headless_ticks_and_determinism`, `replay.record_and_replay_match_every_tick`, `tests/game/determinism/keep_walk.lua` (CTest `determinism_keep_walk_identical`) |
| simulation-core | Divergence is detected | `world.state_hash_detects_divergence` |
| simulation-core | Record and replay | `tests/game/determinism/keep_walk.lua` (CTest `determinism_keep_walk_replay`), `replay.record_and_replay_match_every_tick` |
| simulation-core | Slot reuse | `world.spawn_names_and_slot_reuse` |
| simulation-core | Riding a moving frame | `world.nested_frames_and_reparenting` |
| simulation-core | Changing parent without a jump | `world.nested_frames_and_reparenting` |
| simulation-core | Far from the origin | `core.double_precision_far_from_origin` |
| xr-presentation | Entering VR | pending (task 13.1) |
| xr-presentation | No XR support | pending (task 13.5) |
| xr-presentation | Session ended by the headset | pending (task 13.1) |
| xr-presentation | Leaning toward a wall | pending (task 13.3) |
| xr-presentation | Pulling the lift lever | `tests/game/scenarios/keep_lift.lua` (the use path an XR ray triggers); XR controller input pending (task 13.2) |
| xr-presentation | Teleport movement | pending (task 13.4) |
| xr-presentation | Comfort defaults | pending (task 13.4) |
