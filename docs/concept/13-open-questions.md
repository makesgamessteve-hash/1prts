# 13 · Open Questions

These decisions are still open. Each one has a **default** that the concept and specs already use,
so work can continue. Changing one later means updating the concept and, where marked, the specs.

| # | Question | Default (in use now) | Alternatives | Affects |
|---|---|---|---|---|
| Q1 | Is **"1stPRTS"** the final title? | Working title "1stPRTS", executable `1stprts` | A new name | Branding only |
| Q2 | May the commander **take crew stations** (drive, gun, fly)? | Yes, with costs (radio only, the crew member steps aside) | Never; or passenger only | M3 spec |
| Q3 | **Succession** rules | 3 successors, 30 s respawn at a succession anchor, 60 s handover | Permadeath (one life); unlimited successors | M4 spec |
| Q4 | **Command pause** in single player | Optional match rule, on for Normal, off for "Iron Commander" | Never pause; always allowed | M2 spec |
| Q5 | **Setting and factions** | Veyra and the Ninth Clause; Directorate against Free Hollows Compact; Custodians later | Another setting or tone | Concept and content |
| Q6 | **Multiplayer** timing | After 1.0; the simulation is kept deterministic per build so this stays possible | Plan versus mode for M4–M5 | Architecture (network model) |
| Q7 | **Native VR** (OpenXR) on desktop | Not planned. WebXR only; the renderer is ready for stereo | Add an OpenXR backend in M7 | M7 |
| Q8 | Default **logistics** model | Physical for M/L skirmish, simplified for S and quick matches | Always physical; always simplified | M4 spec |
| Q9 | Should touch and WebXR come in **M1** already? | Yes, basic versions in M1 to remove platform risk early; polish in M7 | Desktop only first, the rest in M7 | M1 spec (in the current proposal) |
| Q10 | **Lua version** | Lua 5.4, which also runs on the web | LuaJIT on desktop only (two runtimes) | M1 spec |
| Q11 | **Voxels** | Used for Custodians, `.vox` prop import, and later optional deformable terrain | Leave voxels out | M1 adds `.vox` loading |
| Q12 | **Art production** | Programmer art and procedural placeholders under CC BY 4.0; community contributions welcome | Commission an artist; an asset style guide first | Schedule |
| Q13 | **Native mobile apps** (iOS/Android stores) | No; mobile is played in the web browser | Native SDL3 builds for mobile later | Build and CI |
| Q14 | Minimum **coverage scope** | ≥ 95% line coverage of engine C code and base-game Lua, with a short list of reviewed exclusions (platform entry points, JS glue) | Count everything, including platform glue | M1 spec |
