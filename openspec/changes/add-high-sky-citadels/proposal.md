# Proposal: Skyfall (M6), the storm layer and the high sky

## Why

The concept's late game is fought above the clouds (concept 04 §3):
- the floating citadels of the High Imperium still drift on slow tracks over the land;
- a side that brings aether and casters to a citadel's core can wake it, steer it and rule from
  its throne hall.

Between the sky and the high sky lies the storm layer: lightning, wind and near-zero sight.
Story act V (concept 15) raises the fallen citadel Caldrith out of the steaming Caldmere with
Ashby aboard: *Storm the Citadel*. M6 brings the citadels, the storm layer, the first casters and
far-sight, the tier-6 flyers and that scenario.

## What Changes

- **Floating citadels** (scripts): a citadel is a moving frame built from a deck plan:
  - courtyard, docks and drop-gates;
  - a throne hall with a war table and a **scrying floor** (a live view of the land below);
  - under the rock, the **aether core**, where the commander floats.

  Dormant, a citadel drifts along a looping track. A side wakes it by bringing a caster aboard and
  paying aether at the core's aetherstone. A woken citadel:
  - is steered slowly from its throne hall's war table;
  - sees far below it;
  - drops its garrison through the drop-gates;
  - bombards what lies beneath, for aether.

  Skyships dock at a citadel and hand over the troops they carry.
- **The storm layer** (scripts): a map can have a cloud deck, a band of heights where:
  - sight drops to a few dozen metres;
  - winds push flyers and skyships;
  - lightning strikes what flies there.

  The commander inside it sees the fog close in.
- **Casters and spells** (scripts): a spell system that costs aether, has cooldowns and needs
  sight. The first casters:
  - the Crown's **Magister** (Firebolt), trained at the new **Arcanum**;
  - the Wildborn's **Bone-seer** (Wither), trained at the new **Spirit lodge**.

  Both buildings perform the **far-sight** ritual: a wide area is seen once, with a long cooldown.
  They are the far-sight buildings the tier-6 mandates need.
- **Tier 6:**
  - the Crown's **sky galleon**, a heavy flying warship with guns and a drop bay, built at the new
    **Sky mast**;
  - the Wildborn's **elder drake**, a fire-breathing flyer from the roost cliffs.

  Both reach the high sky and can board or attack a citadel.
- **Storm the Citadel** (scenario map *Caldmere*):
  - Caldrith rises out of the crater lake;
  - the commander flies up through the storm by skyship with a company and a Magister;
  - Wildborn boarders come on drakes and wyverns;
  - the Crown wins by waking Caldrith with Ashby and holding it clear of boarders, and loses if
    the boarders hold its core long enough to break the aetherstone.
- **Skirmish citadel phase:** Thornmere (an M map) gains a dormant citadel on a track over the
  field.

Engine (C):
- `prts.scene.aim_surface`, which moves a surface's camera without rebuilding the scene;
- a fog-only `prts.scene.env` no longer rebuilds the scene.

## Impact

- All platforms. Citadels reuse moving frames, deck plans, posts, the war table and cargo from
  M3 and M5.
- Saves: all new state is entity data. No format change.
- Tests: a game test per spec scenario; C and Lua coverage gates.
