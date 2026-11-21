# Spec Delta

## Purpose

Casters, spells that cost aether, and the far-sight ritual.

## ADDED Requirements

### Requirement: Spells
A caster SHALL cast a spell at a target in range that its side sees, paying the spell's aether; it SHALL NOT cast during the spell's cooldown or without the aether.

#### Scenario: Firebolt
- **WHEN** a Magister casts Firebolt at an enemy timber building in range
- **THEN** the building is damaged and set alight, and the side's aether goes down by the spell's cost

#### Scenario: No aether, no spell
- **WHEN** a caster tries to cast with too little aether, or again during the cooldown
- **THEN** the spell fails and the caster says why

#### Scenario: Wither
- **WHEN** a Bone-seer casts Wither on a company of the enemy
- **THEN** the men in the area lose health over the following seconds

### Requirement: Far-sight
The Arcanum and the Spirit lodge SHALL perform far-sight: everything in a wide area is seen once, then the ritual cools down.

#### Scenario: Far-sight
- **WHEN** a side's Arcanum performs far-sight on a distant enemy camp
- **THEN** the camp's buildings and units appear in the side's picture, and far-sight cannot be performed again until its cooldown passes
