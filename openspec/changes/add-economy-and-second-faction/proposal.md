# Proposal: Hold and Harvest (M4), the full economy and the Wildborn

## Why

M2's skirmish is one faction with two resources. The concept's economy has more:
- six resources per faction, two of them the faction's own;
- tiers reached by growing the headquarters, and costly mandates beyond them;
- workshop upgrades;
- buildings that rise in stages and burn;
- a commander whose presence matters and who can fall.

It also has a second faction that plays differently (concept 05, 06, 08). M4 brings these, so a
skirmish is Crown against Wildborn with a real economy.

## What Changes

- **Factions (scripts).** The rules no longer name Crown buildings. Each faction is a definition
  with roles: headquarters by tier, worker, food building, foot hall, cavalry hall, tower,
  timber drop, workshop, aether site and AI plan. Sides have a faction, and the AI, the war table
  and the tools work for any faction.
- **Resources.**
  - Gold and timber, as before.
  - **Aether** from wells and shrines built on aether springs: slow and steady.
  - **Food** caps **folk**, as before.
  - **Powder** (Crown only) is made at the Dwarven Forgeworks from gold and timber.
  - **Bone** (Wildborn only) comes from herd pens, and Kobb gather far more from remains left
    where units fell.
- **Tiers.**
  - T1–T3: upgrading the headquarters (Holdfast → Keep → High Castle; Longhouse → War Lodge →
    Antler Throne). Each stage is a bigger building, and units and buildings need a tier.
  - T4–T6 are **mandates** (Royal Charters, Great Rites) bought at the T3 headquarters. Each
    needs certain buildings first. Their content (casters, flyers, signature units) fills in with
    M5–M7; the mechanism and the requirements are here.
- **Upgrades** at workshops (smithy, bone forge): sharper blades, fire bolts, bone-tipped spears.
- **Construction stages:** staked out → foundations → frame and scaffolding → walls and roof →
  finished. Each stage shows, and presence speeds building by 25%.
- **Timber and stone:**
  - Timber buildings burn: fire hurts them, spreads to timber neighbours and is put out by
    workers.
  - Stone buildings do not burn.
  - Kobb firebrands and the Crown's fire bolts set fires.
- **Presence:** within 50 m of the commander, their side's companies hold longer and hit harder,
  builders build 25% faster and firefighters work 20% faster.
- **Succession:**
  - The commander has health and enemies attack them. At no health they fall.
  - After a delay they wake as their successor in the war room of the best succession anchor
    (a headquarters from the second stage, or a behemoth). The stones are silent for a
    handover period.
  - With no successors or anchors left, the side is defeated.
- **The Wildborn Clans:**
  - Kobb workers; Ghorr brutes, spearhurlers and Kobb firebrands from the war pit; wolf runners
    and boar riders from the beast den; ridge giants from the giant's cairn.
  - Herd pens, timber camp, bone forge, totem watch, aether shrine.
  - The **behemoth**: a moving war lodge. It walks where it is ordered, takes deliveries, is a
    stone hub and a succession anchor, and carries a war room in its howdah that the commander
    can climb up to.
- **The AI** plays either faction from its plan:
  - it builds through the tiers, buys upgrades and sets up aether;
  - it expands with a second headquarters near fresh mines when its own run low.
- **A medium map, *Thornmere*:** Crown in the west, Wildborn in the east, mines and aether
  springs for both and in the middle. It is in the menu.

Engine (C): none planned. Everything is scripts and data.

## Impact

- All platforms as before; the war table and stone menus show the faction's own buttons.
- Saves: all state is entity data (saved and hashed).
- Tests: a game test per spec scenario, and Lua coverage over the new modules.
