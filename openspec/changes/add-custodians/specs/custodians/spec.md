# Spec Delta

## Purpose

The Custodians: the voxel-built guardians of the citadels, a fifth side without a commander.

## ADDED Requirements

### Requirement: Constructs of voxels
Custodian units SHALL be drawn from voxel models, not from sprites.

#### Scenario: A warden made of voxels
- **WHEN** Custodians wake at a ruin
- **THEN** their wardens are drawn from a `.vox` model and have no sprite

### Requirement: A side without a commander
The Custodians SHALL have no headquarters and no workers, SHALL never win or lose a match, and SHALL NOT be chosen as an AI's war foe.

#### Scenario: Custodians do not end a match
- **WHEN** the Custodians share a match with two sides and one side's headquarters falls
- **THEN** the other side wins, and the Custodians are neither winner nor loser

### Requirement: Guardians of their wards
Custodians SHALL fight what comes into the ward round their post and SHALL return to their post when it is clear; hostile Custodians SHALL also raid the nearest headquarters.

#### Scenario: Driven from the ward
- **WHEN** a company walks into a neutral warden's ward
- **THEN** the Custodians attack it, and when it has fled the warden walks back to its post

#### Scenario: A hostile raid
- **WHEN** hostile Custodians have stood for a raid's interval
- **THEN** their wardens march on the nearest headquarters

### Requirement: Repairing the land
A Custodian spire SHALL reclaim aether wells near it so that they give their owner nothing, and Custodians SHALL rebuild an uncontested ruin into a spire.

#### Scenario: Reclaiming a well
- **WHEN** a spire stands within reach of a side's aether well
- **THEN** the well stops giving that side aether, and the side is told

#### Scenario: Rebuilding a ruin
- **WHEN** Custodians hold a ruin with no enemy near for a minute
- **THEN** a spire rises at the ruin

### Requirement: Waking with the citadels
Custodians SHALL wake aboard citadels and SHALL be offered as an option in skirmish.

#### Scenario: Guardians aboard a citadel
- **WHEN** Custodians are woken aboard a citadel
- **THEN** they stand in its courtyard and core and move with it

#### Scenario: Hostile Custodians in a skirmish
- **WHEN** a skirmish starts with the Custodians option set to hostile
- **THEN** the Custodians hold the map's Imperium ruins as a fifth side
