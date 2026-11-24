# Design: The Skyfall Wars campaign (M10)

## Acts (`game/campaign.lua`)

`Ca.ACTS` is plain data, so mods can replace it. Each act is
`{ id, title, map, goal, briefing = { { speaker, text } }, won = "match" | "orders" | "ending" }`.

| id | title | map | won when |
|---|---|---|---|
| `prologue` | Prologue: Harrowmark | `proving_ground` | the castellan's four orders are done |
| `act1` | I - The Silent Watch | `ashford_vale` | the rival lord's holdfast falls |
| `act2` | II - The Border Marches | `greyfen_march` | Lord Varric's fort falls |
| `act3` | III - Timber and Gold | `thornmere` | the clans' war lodge falls |
| `act4` | IV - The Saltcoast | `saltcoast` | the Harbor Siege is broken |
| `act5` | V - Above the Clouds | `caldmere` | Caldrith is woken by the Crown |
| `act6` | VI - The Hollow Roads | `underhill` | the Hollow Court falls |
| `act7` | VII - Deep Water | `reefholm` | the Tidehold sinks |
| `act8` | VIII - The Custodians Wake | `skyfall_meres` | an ending is chosen |

## Progress

- **Done acts.** `campaign.done.<id> = true` in the settings, written with
  `prts.settings.save`. `campaign.ending` holds the Act VIII choice.
- **Unlocking.** An act is open when it is the first or the act before it is done.
- **Continue.** It plays the first open act that is not done (the last act when all are done).

## Starting an act

- **`Ca.start(id)`:**
  - with no map loaded, starts the act's map at once (`session.start(map, { campaign = id })`);
  - while a map is loaded, writes `campaign.pending = id`, saves, and calls
    `prts.app.restart(map)`.
- **At boot.** `scripts/main.lua` passes `{ campaign = pending }` when the pending act's map is
  the boot map, and clears `campaign.pending`.
- **`Ca.begin(id)`** (on `session_started` with `opts.campaign`):
  - remembers the act;
  - shows its title card with the goal;
  - plays its briefing with `ui/subtitles.lua`.

## Winning and losing

- **The events.**
  - `rts_match_over(winner)`: winner 1 completes a `match` act; anything else fails it.
  - `order_done(_, count)`: completes the prologue when all its orders are done.
  - `campaign_ending(choice)`: completes Act VIII.
- **`Ca.complete`:**
  - marks the act done and saves;
  - shows "Act complete" with the next act's title;
  - emits `campaign_act_done`.
- **`Ca.fail`:**
  - shows "The act is lost";
  - emits `campaign_act_failed`.
- **The pause menu** then offers "Next act: <title>" or "Try the act again", and "Campaign".

## Act VIII (`maps/skyfall_meres.lua`)

- **The map.**
  - It reuses the Caldmere's crater, camp and mast (`maps/caldmere.lua`'s `build`) and adds three
    Imperium ruins on the shores.
  - Caldrith is spawned woken by the Crown, high over the lake. Two other citadels (Vael Tor and
    Orm Sedd) drift on tracks.
- **The sides.**
  - The Crown (side 1) holds the camp. Caldrith's garrison is 12 men-at-arms, 8 crossbowmen and a
    Magister.
  - The Custodians join hostile, holding the ruins, and wake aboard every citadel. Aboard
    Caldrith, the Keeper (a colossus) and its wardens hold the core.
- **The council.** Urgra Ashtusk, High Delver Ilvane and Tide-Admiral Jory Salt stand in the
  throne hall, with Ashby at the stone. Each speaks their people's mind.
- **The start.** The commander starts at Caldrith's war table.
- **The choices.** Three usable things in the throne hall and the core:
  - the helm by the throne: steer;
  - the council stone at the table: share;
  - the aetherstone's ledge in the core: sink.

  While Custodians remain aboard Caldrith, each answers "The guardians still hold the core".
- **The endings.** Each plays its lines, shows its title card, ends the match for the Crown and
  emits `campaign_ending(choice)`:
  - *The Crown's Sky*: the Custodians bow to the helm and stand down;
  - *The Concord of the Meres*;
  - *The Long Calm*: the stone breaks, the citadels sink, and the Custodians sleep.
- **Losing.** The act is lost if Caldrith's stone breaks (Custodians cannot break it; others
  can) or if the camp's keep falls.

## Restarting (engine)

- **`prts.app.restart(map)`** keeps the request (lowercase letters, digits, `_`):
  - interactive runs stop their frame loop after the frame;
  - test, batch and scenario runs only keep it (`prts.app.restart_pending()`).
- **The launcher** (`apps/1stprts/main.c`) destroys the engine and creates it again from the same
  configuration with `start_map` set to that map. It re-attaches the web hooks on the web.
