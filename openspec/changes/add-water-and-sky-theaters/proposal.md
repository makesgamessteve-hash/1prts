# Proposal: Sail and Wing (M5), water and sky

## Why

So far every fight is on land. The concept's war is also fought on the water and in the sky,
and the commander can stand on all of it:
- the quarterdeck of a galleon;
- the chart room below it;
- the basket of a balloon;
- the deck of a skyship (concept 04, 05).

The story's fourth act breaks the Wildborn siege of the Saltcoast by ship and by sky: the
*Harbor Siege* (concept 15). M5 brings water, ships, harbors, flyers and that scenario.

## What Changes

- **Water (scripts, a little engine):**
  - A map can have a sea level. Water is drawn as a surface and is deep where the ground lies
    below it: the commander wades, slowly, and the navigation grid closes over deep water (a new
    engine call blocks grid cells whose ground lies below a height).
  - Ships keep to water deep enough for their draft.
- **Ships**, moving frames built from deck plans, which the commander walks while they sail:
  - Crown: the cog (a transport that embarks and lands companies), the caravel (fast warship)
    and the war galleon (hold, gun deck, main deck, quarterdeck and a **chart room** with a war
    table: a naval command post);
  - Wildborn: the longship (raider and transport) and the ram galley (rams other hulls).
  - Ships fight with broadsides (a ship's guns fire to either side at ships and buildings in
    range) or rams.
  - **Damage control:** hits fall on rooms; hull rooms below the waterline let water in; the crew
    at the pumps bail it out; a ship that fills sinks. The commander's presence on the
    quarterdeck speeds the reload.
- **Harbors:** the Crown's harbor and the Wildborn raft-yard stand on a shore and launch ships.
  The harbor is also the building the tier-5 mandate needs.
- **The sky:**
  - the Crown's **observation balloon** on its winch rises with the commander in its basket and
    sees far;
  - the Wildborn's **wyvern riders** fly over land and water, hunt balloons and riders, and only
    missiles reach them;
  - the **dwarven skyship** flies with the commander on its deck.
- **The Harbor Siege** (a scenario map, *Saltcoast*):
  - Clan Varrak besieges the Crown's harbor town;
  - the relief comes up the bay in a war galleon, a caravel and two cogs with troops, and over the
    hills in a skyship;
  - the commander starts in the galleon's chart room;
  - the Crown wins when the siege camp's war lodge falls, and loses if the harbor town's keep
    falls or the galleon sinks.

Engine (C): `prts.nav.block_below`. Everything else is scripts and data.

## Impact

- All platforms; ships and flyers reuse moving frames, posts, deck plans and crews from M3 and the
  rules from M2 and M4.
- Saves: all new state is entity data. No format change.
- Tests: a game test per spec scenario; C and Lua coverage gates.
