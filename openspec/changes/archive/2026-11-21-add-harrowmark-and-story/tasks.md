# Tasks: Harrowmark, a story and a title

## 1. Castle

- [x] 1.1 Add hollow round walls (tube) to the scene with open sides and closed edges; verify unit tests that every face looks the right way (`shapes.towers_face_out_and_in_with_open_sides`, also `shapes.wedges_face_outwards_in_every_direction`)
- [x] 1.2 Write the castle building blocks in Lua (battlements, curtain walls with wall-walks, round towers with doors, floors and an optional spiral stair, wall rings, gatehouses, wall stairs, halls) and the portcullis; verify Lua unit tests (`scripts/tests/test_castle.lua`)
- [x] 1.3 Build Harrowmark around the keep (inner and outer ward, Raven Tower, Lion Gate, King's Gate, halls, garrison, banners) and move the start before the King's Gate; verify the scenarios `castle_walls.lua`, `castle_gate.lua` and `raven_tower.lua` (spec *Onto the wall-walk and into a tower*, *To the top of the great tower*, *Closing the gate*, *Approaching the castle*)
- [x] 1.4 Update the tour, the golden images and the screenshots; verify `render_golden` and `render_golden_catches_fog_change` pass with the new references

## 2. Story and title

- [x] 2.1 Rename the game to "Banner & Bastion" in every player-facing place (window title from `mod.lua`, menu, web page, credits, README and concept); verify `scripts/tests/test_story.lua` and the menu scenario (spec *Front-end menu*)
- [x] 2.2 Write `docs/concept/15-story.md` (premise, places, cast, campaign acts, other factions' view, storytelling rules)
- [x] 2.3 Add HUD subtitles, the prologue (menu starts only, with an option to turn it off), people who talk and the castellan's orders; verify `test_story.lua`, `story_prologue.lua` and `npc_talk.lua` (spec *Starting Harrowmark from the menu*, *Talking to a guard*, *Reading the board*)
- [x] 2.4 Add use-only boxes and a separate script budget for data files; verify `collide.use_boxes_stop_only_the_use_ray` and `script.data_gets_its_own_budget_inside_a_long_script`
