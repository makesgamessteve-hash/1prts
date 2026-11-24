# Proposal: The Skyfall Wars campaign (M10)

## Why

The story of *Banner & Bastion* has been prototyped act by act: each milestone added one act's
first mission as a scenario (concept 15 §5). Release 1.0 ships them as one campaign:
- played in order;
- with progress that is kept;
- told through the commander's eyes and ears;
- ending in Act VIII, *The Custodians Wake*, where the four peoples must choose what becomes of
  the citadels.

## What Changes

- **A campaign screen** on the front-end menu:
  - the prologue and the eight acts in order, each unlocked by the one before;
  - Continue starts the first act not yet done.
- **Each act is a mission on its map:**
  - a title card;
  - a briefing over the sending stone;
  - its goal;
  - won when the Crown wins (the prologue: when the castellan's orders are done).
- **Progress kept:**
  - a won act is marked done in the player's settings, with the ending chosen in Act VIII;
  - the pause menu offers the next act, or the act again after a defeat.
- **Moving between acts:** `prts.app.restart(map)` starts the game again on another map (one map
  per run is the engine's rule). The act to begin is handed over in the settings.
- **Act VIII, *The Custodians Wake*** (a new map, `skyfall_meres`):
  - Caldrith, woken by the Crown, hangs over the Caldmere, and the Custodians wake with it and
    with every drifting citadel;
  - the Marshal must clear the guardians from Caldrith's core, then decide at the throne hall
    among three choices, each with its own ending:
    - steer the citadels for the Crown;
    - share them in a concord of the four peoples;
    - break the stone and sink them.
  - The act is lost if Caldrith falls.

## Impact

- New:
  - `game/campaign.lua`;
  - `maps/skyfall_meres.lua`;
  - `prts.app.restart`, `prts.app.restart_pending` (engine) and the launcher's restart.
- Changed:
  - `ui/menu.lua` (Campaign screen, pause menu entries);
  - `scripts/main.lua` (an act handed over at boot);
  - `game/session.lua` (the map list).
