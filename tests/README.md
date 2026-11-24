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
| `tests/fun/` | The fun audit's instruments: a mod with telemetry, bot personas, Harrowmark's route graph and probes (never shipped), the persona episodes and the survey | `python3 tools/fun/audit.py` (see `tools/fun/README.md`); CTest `fun_survey` |
| `tools/tests/` | Self-tests of the repository checks (asset validator, API documentation and spec coverage checks) | `python3 -I tools/tests/test_validate_assets.py` and `test_check_scripts.py` in the CI `checks` job |

```sh
cmake --preset dev && cmake --build --preset dev
ctest --preset dev                      # render_tour needs a display (CI uses xvfb-run)
build/dev/bin/prts_tests --filter='replay.*'
ctest --preset dev -L smoke            # the commander-is-there smoke set (walk, stairs, door, lift, war table)
ctest --preset dev -LE slow            # everything but the long AI matches (label `slow`; CI's sanitizer job skips them)
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
| asset-filesystem | Changing an asset without rebuilding | `vfs.mod_overrides_and_case` (content is read from the data folder at run time), `gfx.windowed_engine_renders_and_recovers_from_broken_content` (a mod's files replace the base ones without a rebuild) |
| asset-filesystem | Data folder missing | `engine.boot_errors` |
| asset-filesystem | Mod overrides a base file | `vfs.mod_overrides_and_case`; `data/base/scripts/tests/test_audio.lua` (every door and lift sound path loads) |
| asset-filesystem | Mod adds a new file | `vfs.merged_listing` |
| asset-filesystem | Incompatible mod | `engine.mods_are_checked_and_mounted`, `mods.dependencies_and_load_order` |
| asset-filesystem | Path escape attempt | `vfs.path_rules`, `script.vfs_api` |
| asset-filesystem | Same behavior across platforms | `vfs.mod_overrides_and_case` |
| asset-filesystem | Settings persist | `vfs.user_area` (storage), `settings.video_preset_survives_a_restart` |
| asset-filesystem | Missing model | `model.missing_and_broken_models_show_a_placeholder`, `engine.game_api_from_scripts` (`prts.scene.model` with a missing path) |
| asset-filesystem | Credits screen | `tests/game/scenarios/front_end_menu.lua` (with the `credits_demo` mod) |
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
| automated-testing | Windows-only failure | `.github/workflows/ci.yml` (the windows job builds and runs every test on Windows and fails the run on its own) |
| commander-locomotion | Release build | `console.release_builds_have_no_way_out_of_the_commanders_eyes`, `tests/game/scenarios/dev_console.lua` |
| commander-locomotion | Developer observer camera | `tests/game/scenarios/dev_console.lua` |
| commander-locomotion | Walking speed | `tests/game/scenarios/walk_speed.lua`, `commander.walking_speed_and_crouch` |
| commander-locomotion | Crouching lowers the view | `commander.walking_speed_and_crouch` |
| commander-locomotion | Stairs | `tests/game/scenarios/spiral_stair.lua` (the keep's spiral stair to the roof), `tests/game/scenarios/keep_stairs.lua` |
| commander-locomotion | Wall at sprint speed | `tests/game/scenarios/sprint_into_wall.lua`, `collide.sliding_stepping_and_ground`, `collide.slope_limit_on_terrain` |
| commander-locomotion | Opening a door | `tests/game/scenarios/keep_door.lua` |
| commander-locomotion | Riding the keep's hoist lift | `tests/game/scenarios/keep_lift.lua`, `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Walking during movement | `commander.rides_a_moving_platform_and_uses_things` |
| commander-locomotion | Reaching the war room | `tests/game/scenarios/keep_lift.lua`, `tests/game/scenarios/war_table.lua`, `tests/game/determinism/keep_walk.lua` |
| input-actions | Mod adds an action | `engine.game_api_from_scripts`, `data/base/scripts/tests/test_input.lua` |
| input-actions | Rebinding use | `settings.rebinding_use_survives_a_restart` |
| input-actions | Gamepad hot-plug | `platform.events_become_device_state` |
| input-actions | Phone browser | `web/tests/mobile.test.js` (an emulated phone starts in the touch scheme with the overlay drawn), `engine.status_for_test_hooks_and_scheme_hint` |
| input-actions | Tablet with a keyboard | `tests/game/scenarios/touch_controls.lua`, `web/tests/mobile.test.js` (a key switches an emulated phone to keyboard and mouse) |
| input-actions | Touch movement and look | `touch.move_and_look_at_the_same_time`, `tests/game/scenarios/touch_controls.lua`, `web/tests/mobile.test.js` (browser touch events on both halves at once) |
| input-actions | Prompt update | `input.schemes_and_overrides`, `tests/game/scenarios/hud_prompt.lua` (the scheme-changed event and the gamepad prompt) |
| lua-scripting | Replacing the boot script | `engine.mods_are_checked_and_mounted` |
| lua-scripting | Stale entity handle | `data/base/scripts/tests/test_input.lua` |
| lua-scripting | Blocked operating system access | `script.sandbox_blocks_os_access` |
| lua-scripting | Module loading through the file system | `script.require_through_vfs_with_overrides` |
| lua-scripting | Error in an event handler | `script.error_containment_in_event_handlers` |
| lua-scripting | Endless loop | `script.budget_interrupts_endless_loop` |
| lua-scripting | Same seed, same random numbers | `script.deterministic_random_and_time` |
| lua-scripting | Changing a door script while playing | `reload.changing_a_door_script_while_playing`, `reload.modules_are_rerun_in_place_and_on_reload_runs` |
| lua-scripting | Running a command | `console.commands_run_in_the_game_environment`, `tests/game/scenarios/dev_console.lua` |
| platform-targets | Build on each desktop platform | `.github/workflows/ci.yml` (linux, windows and macos jobs) |
| platform-targets | OpenGL context unavailable | `platform.impossible_gl_version_is_reported` |
| platform-targets | Web bundle runs in a browser | `web/tests/boot.test.js` |
| platform-targets | Browser without WebGL 2 | `web/tests/no_webgl2.test.js` |
| platform-targets | Same scenario result on desktop and web | `web/tests/smoke.test.js` (the smoke set, `tests/game/scenarios/keep_lift.lua` among it, run in the browser must pass with the `final_cell` the desktop CTest run ends in: CTest `scenario_keep_lift`, label `smoke`), `engine.scenario_result_with_final_cell_and_hash` |
| platform-targets | Extract and run | `tools/package_smoke.py` (CI `package` job on each desktop platform) |
| platform-targets | Licenses included | `tools/package_smoke.py` (CI `package` job) |
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
| audio-playback | Pitch changes the playback rate | `audio.pitch_changes_the_playback_rate` |
| automated-testing | Auditing a build | `tools/fun/audit.py` (a 20-episode run per persona; reports kept in `docs/fun-audit/`), `tools/tests/test_fun_metrics.py` |
| automated-testing | A way through the map is broken | `tests/fun/survey.lua` (CTest `fun_survey`; on the commit before the stair fix it fails naming `stair_foot - war_stair`) |
| automated-testing | Same light, less waiting | `collide.bvh_matches_checking_every_box` (occlusion against brute force), `tests/golden/golden.lua` (CTest `render_golden`: the references are unchanged), `tests/fun/probe_frames.lua` (the audit's load time) |
| castle-content | The keep's stairs to the war room | `tests/game/scenarios/keep_war_room_stairs.lua`, `tests/fun/survey.lua` |
| commander-locomotion | Speeding up and slowing down | `commander.movement_feel_buffer_coyote_and_easing`, `commander.walking_speed_and_crouch`, `tests/game/scenarios/commander_feel.lua` |
| commander-locomotion | Jump pressed just before landing | `commander.movement_feel_buffer_coyote_and_easing`, `tests/game/scenarios/commander_feel.lua` |
| commander-locomotion | Jump just after the edge | `commander.movement_feel_buffer_coyote_and_easing`, `tests/fun/probe_feel.lua` (the audit's 40 human-timed jumps) |
| commander-locomotion | Hang time | `commander.movement_feel_buffer_coyote_and_easing` |
| commander-locomotion | Feel stays out of VR | `commander.movement_feel_buffer_coyote_and_easing` |
| commander-locomotion | Footsteps and landings | `tests/game/scenarios/commander_feel.lua`, `data/base/scripts/tests/test_feel.lua` |
| commander-locomotion | Camera motion setting | `tests/game/scenarios/commander_feel.lua`, `data/base/scripts/tests/test_feel.lua` |
| commander-locomotion | Heavy things nearby | `data/base/scripts/tests/test_feel.lua` (the slam and the shake), `tests/fun/probe_feel.lua` (the shake stays under 4% of the view) |
| storytelling | Coming through the King's Gate | `tests/game/scenarios/castellan_orders.lua`, `data/base/scripts/tests/test_feel.lua` |
| storytelling | Doing the castellan's orders | `tests/game/scenarios/castellan_orders.lua`, `data/base/scripts/tests/test_feel.lua` |
| storytelling | Talking to a guard turns them | `tests/game/scenarios/npc_talk.lua`, `data/base/scripts/tests/test_feel.lua` |
| navigation | Around a wall | `engine/tests/test_nav.c` (`nav.paths_go_around_walls_and_buildings`), `data/base/scripts/tests/test_nav.lua` |
| navigation | A building blocks the way | `engine/tests/test_nav.c` (`nav.paths_go_around_walls_and_buildings`, `nav.cliffs_islands_and_saved_blocks`), `tests/game/scenarios/rts_gather.lua` (a felled tree frees its ground) |
| navigation | Eight soldiers to one point | `engine/tests/test_nav.c` (`agents.eight_soldiers_reach_one_point_apart`) |
| rts-core | A serf brings gold home | `tests/game/scenarios/rts_gather.lua` |
| rts-core | A barracks trains men-at-arms | `tests/game/scenarios/rts_train.lua` |
| rts-core | No food, no soldiers | `tests/game/scenarios/rts_train.lua` |
| rts-core | Crossbowmen against men-at-arms | `tests/game/scenarios/rts_battle.lua` |
| rts-core | An order through the stones | `tests/game/scenarios/rts_orders.lua` |
| rts-core | Out of range | `tests/game/scenarios/rts_orders.lua` |
| rts-core | An enemy seen, then lost | `tests/game/scenarios/rts_picture.lua` |
| rts-core | The AI commander takes a holdfast | `tests/game/scenarios/rts_ai_match.lua`, `tests/game/scenarios/rts_command_edges.lua` (the AI defends) |
| command-tools | Ordering a company from the war table | `tests/game/scenarios/war_table_orders.lua` |
| command-tools | Opening the field map | `tests/game/scenarios/field_map.lua` |
| command-tools | Advance to my mark | `tests/game/scenarios/stone_and_staff.lua`, `tests/game/scenarios/rts_command_edges.lua` |
| command-tools | The rally horn | `tests/game/scenarios/stone_and_staff.lua`, `tests/game/scenarios/rts_command_edges.lua` (the other signals) |
| mounts-and-cavalry | Mounting and galloping | `tests/game/scenarios/ride_horse.lua` |
| mounts-and-cavalry | Dismounting | `tests/game/scenarios/ride_horse.lua` |
| mounts-and-cavalry | Knights charge | `tests/game/scenarios/knights_charge.lua` |
| mounts-and-cavalry | Ride with your knights | `tests/game/scenarios/ride_with_knights.lua` |
| vehicles-and-posts | Standing on a wagon | `tests/game/scenarios/wagon_ride.lua`, `engine/tests/test_input.c` (`commander.the_view_turns_with_a_turning_frame`) |
| vehicles-and-posts | Driving the wagon | `tests/game/scenarios/wagon_ride.lua` |
| vehicles-and-posts | Up the tower to the wall | `tests/game/scenarios/siege_tower.lua` |
| vehicles-and-posts | Walking the galleries of a gold mine | `tests/game/scenarios/mine_galleries.lua` |
| deck-plans-and-crews | A small plan | `data/base/scripts/tests/test_deckplan.lua`, `tests/game/scenarios/siege_tower.lua` (its stairs climbed) |
| deck-plans-and-crews | Walking away from the siege tower | `tests/game/scenarios/crew_lod.lua` |
| deck-plans-and-crews | The tower's push room is wrecked | `tests/game/scenarios/room_damage.lua` |
| economy-and-tiers | An aether well on a spring | `tests/game/scenarios/econ_resources.lua` |
| economy-and-tiers | Powder at the Forgeworks | `tests/game/scenarios/econ_resources.lua` |
| economy-and-tiers | Kobb gather bone from the fallen | `tests/game/scenarios/econ_resources.lua` |
| economy-and-tiers | Upgrading the holdfast to a keep | `tests/game/scenarios/econ_tiers.lua` |
| economy-and-tiers | A mandate needs its buildings | `tests/game/scenarios/econ_tiers.lua` |
| economy-and-tiers | Tempered blades | `tests/game/scenarios/econ_tiers.lua` |
| construction-and-fire | A barracks rises in stages | `tests/game/scenarios/build_stages.lua` |
| construction-and-fire | Presence speeds building | `tests/game/scenarios/build_stages.lua` |
| construction-and-fire | Firebrands burn a barracks | `tests/game/scenarios/fire.lua` |
| construction-and-fire | Serfs put the fire out | `tests/game/scenarios/fire.lua` |
| construction-and-fire | Stone does not burn | `tests/game/scenarios/fire.lua` |
| wildborn-clans | Crown against Wildborn | `tests/game/scenarios/rts_wildborn_match.lua` |
| wildborn-clans | A war lodge on the move | `tests/game/scenarios/behemoth.lua` |
| wildborn-clans | The AI expands | `tests/game/scenarios/ai_expands.lua` |
| presence-and-succession | Holding with the commander | `tests/game/scenarios/presence_succession.lua` |
| presence-and-succession | The banner passes to the second | `tests/game/scenarios/presence_succession.lua` |
| presence-and-succession | No one left to lead | `tests/game/scenarios/presence_succession.lua` |
| water-and-ships | Wading at the shore | `tests/game/scenarios/wading.lua`, `engine/tests/test_nav.c` (`nav.deep_water_is_closed`) |
| water-and-ships | A cog lands troops | `tests/game/scenarios/ships.lua` |
| water-and-ships | A broadside | `tests/game/scenarios/naval_battle.lua` (with a ram) |
| water-and-ships | Holed below the waterline | `tests/game/scenarios/damage_control.lua` |
| water-and-ships | The pumps keep her afloat | `tests/game/scenarios/damage_control.lua` |
| water-and-ships | A ship from the harbor | `tests/game/scenarios/harbor.lua` |
| water-and-ships | Orders from the chart room | `tests/game/scenarios/harbor.lua` |
| sky | Up in the basket | `tests/game/scenarios/balloon.lua` |
| sky | Wyverns against a balloon | `tests/game/scenarios/wyverns.lua` |
| sky | Over the hills by skyship | `tests/game/scenarios/skyship.lua` |
| harbor-siege | Breaking the siege | `tests/game/scenarios/harbor_siege.lua` |
| harbor-siege | The galleon lost | `tests/game/scenarios/harbor_siege_lost.lua` |
| citadels | A citadel drifts on its track | `tests/game/scenarios/citadel_track.lua` |
| citadels | Waking the citadel | `tests/game/scenarios/citadel_wake.lua` |
| citadels | No caster, no waking | `tests/game/scenarios/citadel_wake.lua` |
| citadels | Floating in the aether core | `tests/game/scenarios/aether_core.lua` |
| citadels | Steering the citadel | `tests/game/scenarios/citadel_steer.lua` |
| citadels | Seeing from the citadel | `tests/game/scenarios/citadel_steer.lua` |
| citadels | Through the drop-gates | `tests/game/scenarios/citadel_drop.lua` |
| citadels | Bombarding from the sky | `tests/game/scenarios/citadel_drop.lua` |
| storm-layer | Inside the storm | `tests/game/scenarios/storm_layer.lua` |
| storm-layer | Carried by the wind | `tests/game/scenarios/storm_layer.lua` |
| storm-layer | Struck by lightning | `tests/game/scenarios/storm_layer.lua` |
| magic | Firebolt | `tests/game/scenarios/spells.lua` |
| magic | No aether, no spell | `tests/game/scenarios/spells.lua` |
| magic | Wither | `tests/game/scenarios/spells.lua` |
| magic | Far-sight | `tests/game/scenarios/far_sight.lua` |
| high-sky-units | Only at tier six | `tests/game/scenarios/tier_six.lua` |
| high-sky-units | The sky galleon's guns | `tests/game/scenarios/tier_six.lua` |
| high-sky-units | Drakes board a citadel | `tests/game/scenarios/tier_six.lua` |
| storm-the-citadel | Storming Caldrith | `tests/game/scenarios/storm_the_citadel.lua` |
| storm-the-citadel | Caldrith broken | `tests/game/scenarios/storm_the_citadel_lost.lua` |
| storm-the-citadel | A citadel over Thornmere | `tests/game/scenarios/citadel_skirmish.lua` |
| underground | Down through a hole | `tests/game/scenarios/tunnels_walk.lua`, `engine/tests/test_collide.c` (`collide.terrain_holes`) |
| underground | Walking a gallery under the field | `tests/game/scenarios/tunnels_walk.lua`, `engine/tests/test_collide.c` (`collide.under_the_terrain`), `engine/tests/test_nav.c` (`nav.ignores_galleries_under_the_field`) |
| tunnels | Through the tunnel road | `tests/game/scenarios/tunnel_road.lua` |
| tunnels | Delvers dig a tunnel | `tests/game/scenarios/tunnel_dig.lua` |
| tunnels | A gallery collapses | `tests/game/scenarios/tunnel_collapse.lua` |
| tunnels | A flooded gallery | `tests/game/scenarios/tunnel_collapse.lua` |
| tunnels | Into the dark | `tests/game/scenarios/tunnels_walk.lua` |
| tunnels | Down the deep shaft | `tests/game/scenarios/deep_shaft.lua` |
| tunnels | Orders under the ground | `tests/game/scenarios/echo_pillar.lua` |
| gloam-courts | The Gloam economy | `tests/game/scenarios/gloam_economy.lua` |
| gloam-courts | Down to the war room | `tests/game/scenarios/gloam_hq.lua` |
| gloam-courts | Gloam soldiers | `tests/game/scenarios/gloam_units.lua` |
| gloam-courts | The wyrm bursts up | `tests/game/scenarios/tunnel_wyrm.lua` |
| gloam-courts | Four sides, three factions | `tests/game/scenarios/fourway_match.lua` |
| gloam-magic | Glimmer-wright spells | `tests/game/scenarios/gloam_spells.lua` |
| gloam-magic | Mycel keeper spells | `tests/game/scenarios/gloam_spells.lua` |
| under-the-hill | Taking the Hollow Court | `tests/game/scenarios/under_the_hill.lua` |
| under-the-hill | Caldrith's chain | `tests/game/scenarios/deep_shaft.lua` |
| under-water | Diving to the reef | `tests/game/scenarios/diving.lua` |
| under-water | Out of breath | `tests/game/scenarios/diving.lua` |
| under-water | The sky-barge's relic | `tests/game/scenarios/wreck.lua` |
| under-water | Down in the bell | `tests/game/scenarios/diving_bell.lua` |
| brine-league | Pearl divers at work | `tests/game/scenarios/brine_economy.lua` |
| brine-league | Harpoons and hull-borers | `tests/game/scenarios/brine_units.lua` |
| brine-league | A brigantine from the yard | `tests/game/scenarios/brine_units.lua` |
| brine-league | The raft-city sails | `tests/game/scenarios/tidehold.lua` |
| brine-league | Riding the storm winds | `tests/game/scenarios/wind_barque.lua` |
| brine-league | Four peoples | `tests/game/scenarios/four_factions.lua` |
| brine-magic | Weatherwright spells | `tests/game/scenarios/brine_spells.lua` |
| brine-magic | Tidecantor spells | `tests/game/scenarios/brine_spells.lua` |
| neutral-places | Trading at the market | `tests/game/scenarios/market_town.lua` |
| neutral-places | Holding the lighthouse | `tests/game/scenarios/lighthouse.lua` |
| reef-war | Sinking the Tidehold | `tests/game/scenarios/reef_war.lua` |
| reef-war | The harbor falls | `tests/game/scenarios/reef_war_lost.lua` |
| touch-polish | Left-handed with large buttons | `tests/game/scenarios/touch_polish.lua` |
| touch-polish | Buttons under the water | `tests/game/scenarios/touch_polish.lua` |
| xr-hands | Pinching to walk and to use | `web/tests/xr.test.js` (tracked hands) |
| comfort | Riding a ship in VR | `tests/game/scenarios/xr_ride_comfort.lua` |
| performance | A shorter draw distance | `engine/tests/test_engine.c` (`gfx.windowed_engine_renders_and_recovers_from_broken_content`), `tests/game/scenarios/performance.lua` |
| performance | A phone starts on low | `tests/game/scenarios/performance.lua` |
| accessibility | A colour-blind palette | `tests/game/scenarios/accessibility_maps.lua` |
| accessibility | Larger map text | `tests/game/scenarios/accessibility_maps.lua` |
| accessibility | Toggle crouch | `tests/game/scenarios/accessibility_play.lua`, `engine/tests/test_input.c` (`input.toggled_actions_latch`) |
| accessibility | Brightness under the ground | `tests/game/scenarios/accessibility_play.lua` |
| accessibility | Faster game | `engine/tests/test_engine.c` (`engine.game_speed_scales_ticks`), `tests/game/scenarios/accessibility_play.lua` |
| custodians | A warden made of voxels | `tests/game/scenarios/custodians.lua` |
| custodians | Custodians do not end a match | `tests/game/scenarios/custodians.lua` |
| custodians | Driven from the ward | `tests/game/scenarios/custodians.lua` |
| custodians | A hostile raid | `tests/game/scenarios/custodians.lua` |
| custodians | Reclaiming a well | `tests/game/scenarios/custodians_land.lua` |
| custodians | Rebuilding a ruin | `tests/game/scenarios/custodians_land.lua` |
| custodians | Guardians aboard a citadel | `tests/game/scenarios/custodians_citadel.lua` |
| custodians | Hostile Custodians in a skirmish | `tests/game/scenarios/custodians_skirmish.lua` |
| campaign | The campaign screen | `tests/game/scenarios/campaign_menu.lua` |
| campaign | An act is won | `tests/game/scenarios/campaign_acts.lua` |
| campaign | An act is lost | `tests/game/scenarios/campaign_acts.lua` |
| campaign | The next act | `engine/tests/test_engine.c` (`engine.restart_on_another_map`), `tests/game/scenarios/campaign_acts.lua` |
| campaign | The core is held | `tests/game/scenarios/custodians_wake.lua` |
| campaign | Sinking the citadels | `tests/game/scenarios/custodians_wake.lua` |
| campaign | Steering or sharing | `tests/game/scenarios/custodians_wake_endings.lua` |
| editor | A broken map file | `data/base/scripts/tests/test_mapfile.lua`, `tests/game/scenarios/custom_map.lua` |
| editor | Placing in first person | `tests/game/scenarios/editor.lua` |
| editor | Saving a map | `engine/tests/test_engine.c` (`engine.player_text_files`), `tests/game/scenarios/editor.lua` |
| editor | Playing a map | `tests/game/scenarios/custom_map.lua` |
| localization | Switching the language | `tests/game/scenarios/localization.lua` |
| localization | A missing translation | `tests/game/scenarios/localization.lua`, `data/base/scripts/tests/test_i18n.lua` |
| modding-guide | A mod starts with the game | `tests/game/scenarios/example_mod.lua` |
| modding-guide | The example mod works | `tests/game/scenarios/example_mod.lua` |
| localization | Umlauts on screen | `engine/tests/test_ui.c` (`ui.utf8_decoding`, `ui.latin1_text_is_drawn_from_its_block`), `engine/tests/test_engine_ui.c` (`boot.font_metrics_load_ranges_of_characters`) |
