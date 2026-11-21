# Tasks: First Command (M2)

## 1. Navigation and agents (engine)

- [x] 1.1 Navigation grid from the collision world and terrain, A* with deterministic ties, smoothing, run-time blocking; verify `nav.*` unit tests (spec *Around a wall*, *A building blocks the way*)
- [x] 1.2 Agents: path following on the ground, separation, facing, arrival events, proximity queries; state hash and saves (snapshot format 3); verify `agents.*` unit tests (spec *Eight soldiers to one point*)
- [x] 1.3 Lua API `prts.nav`, `prts.agent` with documentation; verify `engine.game_api_from_scripts`-style tests and `tools/check_api_docs.py`

## 2. The rules (scripts)

- [x] 2.1 Crown tier-1 definitions, sides and spawning; verify `rts_train.lua`, `rts_command_edges.lua` (the Lua unit tests run on Harrowmark, which has no skirmish; the rules are covered by the skirmish scenarios)
- [x] 2.2 Gathering, deposits, food and folk; verify `rts_gather.lua` (spec *A serf brings gold home*)
- [x] 2.3 Building and training; verify `rts_train.lua` (spec *A barracks trains men-at-arms*, *No food, no soldiers*)
- [x] 2.4 Combat, morale and destruction; verify `rts_battle.lua` (spec *Crossbowmen against men-at-arms*)
- [x] 2.5 Companies, orders and the sending-stone network; verify `rts_orders.lua` (spec *An order through the stones*, *Out of range*)
- [x] 2.6 The picture of the battlefield; verify `rts_picture.lua` (spec *An enemy seen, then lost*)
- [x] 2.7 The AI commander, matches, victory; verify `rts_ai_match.lua` (spec *The AI commander takes a holdfast*)

## 3. Map and command tools

- [x] 3.1a Ashford Vale (terrain, ford, forests, mines, two holdfasts with war rooms); the menu offers the skirmish
- [ ] 3.1b The Training mission (not done in this pass; the skirmish teaches nothing yet)
- [x] 3.2 The war table and the field map; verify `war_table_orders.lua`, `field_map.lua` (spec *Ordering a company from the war table*, *Opening the field map*)
- [x] 3.3 Sending stone, spyglass and marking staff, horn; verify `stone_and_staff.lua` (spec *Advance to my mark*, *The rally horn*)
- [x] 3.4a Docs (README status, modding API: `prts.ui.pointer`, `prts.commander.tune`), screenshots (`docs/screenshots/30-34`), coverage gates (C 99%, Lua 97%); CI green
- [ ] 3.4b Fun audit with a skirmish persona (not done in this pass: `tools/fun` still plays Harrowmark only)
