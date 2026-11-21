# Tasks: First Command (M2)

## 1. Navigation and agents (engine)

- [ ] 1.1 Navigation grid from the collision world and terrain, A* with deterministic ties, smoothing, run-time blocking; verify `nav.*` unit tests (spec *Around a wall*, *A building blocks the way*)
- [ ] 1.2 Agents: path following on the ground, separation, facing, arrival events, proximity queries; state hash and saves (snapshot format 3); verify `agents.*` unit tests (spec *Eight soldiers to one point*)
- [ ] 1.3 Lua API `prts.nav`, `prts.agent` with documentation; verify `engine.game_api_from_scripts`-style tests and `tools/check_api_docs.py`

## 2. The rules (scripts)

- [ ] 2.1 Crown tier-1 definitions, sides and spawning; verify `scripts/tests/test_rts.lua`
- [ ] 2.2 Gathering, deposits, food and folk; verify `test_rts.lua` and `rts_gather.lua` (spec *A serf brings gold home*)
- [ ] 2.3 Building and training; verify `test_rts.lua` and `rts_train.lua` (spec *A barracks trains men-at-arms*, *No food, no soldiers*)
- [ ] 2.4 Combat, morale and destruction; verify `rts_battle.lua` (spec *Crossbowmen against men-at-arms*)
- [ ] 2.5 Companies, orders and the sending-stone network; verify `rts_orders.lua` (spec *An order through the stones*, *Out of range*)
- [ ] 2.6 The picture of the battlefield; verify `rts_picture.lua` (spec *An enemy seen, then lost*)
- [ ] 2.7 The AI commander, matches, victory; verify `rts_ai_match.lua` (spec *Two AI commanders play a match*)

## 3. Map and command tools

- [ ] 3.1 Ashford Vale (terrain, ford, forests, mines, two holdfasts with war rooms); the menu offers the skirmish and the Training mission
- [ ] 3.2 The war table and the field map; verify `war_table_orders.lua`, `field_map.lua` (spec *Ordering a company from the war table*, *Opening the field map*)
- [ ] 3.3 Sending stone, spyglass and marking staff, horn; verify `stone_and_staff.lua` (spec *Advance to my mark*, *The rally horn*)
- [ ] 3.4 Docs (concept "in the game now", modding API), screenshots, fun audit with a skirmish persona; coverage gates; CI green
