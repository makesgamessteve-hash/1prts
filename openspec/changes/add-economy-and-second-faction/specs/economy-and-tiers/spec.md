# Spec Delta

## Purpose

The full economy of a side: the six resources of each faction, tiers reached by growing the
headquarters and by mandates, and workshop upgrades.

## ADDED Requirements

### Requirement: Aether, powder and bone
A side SHALL gain aether from wells or shrines built on aether springs, the Crown SHALL make powder from gold and timber at the Forgeworks, and the Wildborn SHALL gain bone from herd pens and from remains that Kobb gather where units fell.

#### Scenario: An aether well on a spring
- **WHEN** a side builds an aether well on an aether spring
- **THEN** its aether grows steadily while the well stands

#### Scenario: Powder at the Forgeworks
- **WHEN** the Crown has a finished Forgeworks and gold and timber in store
- **THEN** gold and timber are turned into powder over time

#### Scenario: Kobb gather bone from the fallen
- **WHEN** units fall in battle and Kobb are sent to gather bone
- **THEN** remains lie where they fell and the Kobb carry their bone to a headquarters

### Requirement: Tiers
A side's headquarters SHALL be upgraded through three stages that raise its tier, units and buildings SHALL need their tier, and mandates SHALL raise the tier further when their requirements stand.

#### Scenario: Upgrading the holdfast to a keep
- **WHEN** a Crown side pays for the keep at its holdfast and waits the upgrade time
- **THEN** the holdfast becomes a keep, the side reaches tier 2 and can build the Forgeworks, which it could not before

#### Scenario: A mandate needs its buildings
- **WHEN** a side at tier 3 tries to buy the tier-4 mandate without its required buildings, and again with them
- **THEN** the first is refused and the second raises the side to tier 4

### Requirement: Upgrades
Workshops SHALL research upgrades that make their units stronger, paid and timed like training.

#### Scenario: Tempered blades
- **WHEN** the Crown researches tempered blades at a smithy
- **THEN** its men-at-arms strike harder afterwards
