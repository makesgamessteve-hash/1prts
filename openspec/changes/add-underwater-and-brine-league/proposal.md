# Proposal: Deep Water (M8), under the water and the Brine League

## Why

The last of the concept's eight theaters is under the water (concept 04 §2): reefs, pearl beds
and sunken wrecks, where only divers, diving bells and the Hesk can stay. Its masters, the
**Brine League** (concepts 05 §4, 06, 08, 14), are the fourth faction: traders, sailors and Hesk
who build on every shore and sail a raft-city. Story act VII (concept 15) brings them with the
storms Caldrith stirs up: *the Reef War*. M8 also adds the neutral places the trade needs, market
towns and lighthouses, and full four-faction skirmishes.

## What Changes

- **Under the water** (scripts):
  - **Swimming and diving.** In deep water the commander swims: jump rises, crouch dives and
    walking glides.
  - **Breath.** Under the surface their breath runs out, and they choke and are hurt when it is
    gone. A Brine **breath charm** lasts four times as long.
  - **Air pockets.** Breath comes back at the surface, in a diving bell and in a wreck's trapped
    air.
  - **Underwater fog.** Under the surface the fog closes in, blue-green. One fog arbiter now
    settles the fog of the storm, the tunnels and the water.
- **Reefs, pearl beds and sunken wrecks:**
  - pearl beds are a resource only the League gathers (pearl divers);
  - a sunken wreck of the Imperium holds trapped air and a relic worth aether to whoever dives
    for it.
- **The diving bell:** lowered on its chain from a crane, it carries the commander to the bed of
  the sea. It is full of air and has a small shell table, which makes it a command post under
  the water.
- **The Brine League** (scripts, `rts/brine.lua`), the fourth faction:
  - **Resources:** pearls.
  - **Units:**
    - dockhands and pearl divers;
    - marines, harpooneers (their harpoons pin what they strike), reef guard, hull-borers
      (they strike ships), crab riders and albatross riders;
    - the casters: the **Weatherwright** and the **Tidecantor**.
  - **Ships:** the pearl lighter, sloop, cargo hulk, brigantine, bomb ketch and war carrack.
  - **Buildings:** Wharf hall → Guildhall → Admiralty on a shore, fishery, muster house, timber
    yard, pearl house, shipwright yard, great dock, reef pens, storm tower (storm-eye far-sight),
    tide temple, harpoon battery, signal mast and albatross aerie.
  - **The Tidehold:** the League's raft-city, a headquarters that sails, with a council deck
    and a great war table.
  - **The wind-barque:** an airship that rides the storm layer's winds.
  - **Spells:** Squall, Lightning Lash and Fog Bank (the Weatherwright); Tide Surge, Brine Mend
    and Undertow (the Tidecantor).
  - **AI:** the AI plays the League.
- **Neutral places:**
  - **Market towns** buy and sell resources for gold at prices that move with trade, and the
    League gets better prices. The side that holds a town gets folk and the traders' rumours of
    enemies nearby.
  - **Lighthouses**, held, see far over the sea and relay the sending stone.
- **The Reef War** (scenario map *Reefholm*):
  - the League's Tidehold lies off a reef archipelago with a lighthouse, a market town, pearl
    beds and a sunken sky-barge;
  - the Crown's harbour faces it;
  - the Crown wins by sinking the Tidehold, and loses if its keep falls.
- **Four-faction skirmishes:** *Tidesend*, a large map with a sea in the middle: the Crown, the
  Wildborn, the Gloam and the League.

## Impact

- Scripts and data only; no engine change.
- Saves: all new state is entity data.
- Tests: a game test per spec scenario; coverage gates.
