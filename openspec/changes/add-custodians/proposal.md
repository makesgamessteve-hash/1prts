# Proposal: The Custodians (M10)

## Why

The Custodians are the ancient guardians of the floating citadels (concept 08). Waking a
citadel wakes them too. The campaign's last act (*The Custodians Wake*, concept 15) needs them,
and skirmish gains an optional neutral or hostile fifth side.

## What Changes

- **A fifth side with no commander:** the Custodians have no headquarters, no workers and no
  crews. They never win or lose a match, and the other sides' AI does not pick them as their
  war's foe.
- **Voxel constructs:** every Custodian is drawn from a MagicaVoxel model instead of a sprite,
  so they look clearly unlike the people of Veyra:
  - wardens (heavy melee);
  - lancers (aether bolts);
  - motes (small flyers);
  - the colossus (a walking siege engine);
  - the spire (a pylon).
- **Guardians:**
  - each construct keeps to its ward round the post it woke at;
  - a neutral Custodian fights only what comes into its ward;
  - hostile ones also march on the nearest headquarters now and then.
- **Repairing the land:**
  - a spire reclaims aether wells and shrines near it, so they give their owners nothing;
  - Custodians rebuild a ruin of the Imperium into a new spire when no one contests it.
- **Waking:**
  - `rts/custodians.lua` wakes Custodians at a point, at a ruin or aboard a citadel;
  - a citadel's waking wakes the guardians of the other citadels on the map.
- **Skirmish option:** Custodians off, neutral or hostile (Options, Game). Skirmish maps list
  their Imperium ruins.

## Impact

- New: `rts/custodians.lua`, five voxel models (`tools/gen_models.py`), CREDITS rows.
- Changed:
  - `rts/state.lua` (units drawn from models; Custodians left out of `S.enemies`);
  - `rts/match.lua` (the Custodians never end a match; the skirmish option);
  - `rts/structures.lua` (reclaimed wells give nothing);
  - `game/settings.lua` and `ui/options.lua` (the option);
  - the skirmish maps' ruins.
