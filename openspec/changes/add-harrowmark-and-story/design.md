# Design: Harrowmark, a story and a title

## Building blocks in Lua, one new shape in C
Castles are assembled by `data/base/scripts/lib/castle.lua` from boxes, ramps, spiral stairs and
one new scene shape, the **tube** (a hollow round wall with open sides). Round towers are bands
of tubes cut at door heights; their collision is a ring of boxes around each side (two per side),
which leaves the inside free and keeps every query a box query (design D8 of the engine change).
`C.curtain_ring` places the walls of a ward between its towers and gates and works out each
tower's doors (from the ward on the ground, from every wall that meets it at wall-walk height).

## Scale and cost
Harrowmark adds about 4,000 collision boxes and 150,000 vertices. Static boxes are in the BVH, so
movement and the baked lighting stay fast; boot time with baking is a few seconds on software GL.
Cells and portals are not used for the castle's towers (everything outside the keep is in the
"outside" cell); the 64-cell limit stays free for interiors that need culling.

## Story in first person
Text is shown as HUD subtitles (`ui/subtitles.lua`), timed in ticks so runs stay deterministic.
People are made usable with use-only boxes, which only the use ray and the XR pointer hit, so they
never block movement or cast baked shadows. Dialogue lives in `story/harrowmark.lua`.

## Title and codename
Player-facing text uses "Banner & Bastion"; the window title is read from the base pack's
`mod.lua` (`title`), so a mod or a later rename changes no C code. Technical names (repository,
executable, `prts_` prefix, user-data folder) keep the codename "1stPRTS" to avoid churn and to
keep players' saved settings.

## Script budget for data files
A data file (such as a sprite sheet) loaded while a long script runs gets its own instruction
budget; before, it inherited the caller's count, and the larger castle build crossed the limit.
