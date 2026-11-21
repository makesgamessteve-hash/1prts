# Spec Delta

## Purpose

Floating citadels: drifting on tracks, waking, the aether core, steering from the throne hall,
the scrying floor, drop-gates, bombardment and docking.

## ADDED Requirements

### Requirement: Drifting citadels
A dormant citadel SHALL drift along its looping track at its height, carrying whoever stands on it.

#### Scenario: A citadel drifts on its track
- **WHEN** a dormant citadel is left alone for a while
- **THEN** it moves along its track from point to point and, after the last point, heads for the first again

### Requirement: Waking a citadel
A side SHALL wake a citadel when its commander uses the aetherstone in the core with a caster of the side in the citadel's garrison and the aether to pay; a citadel without a caster aboard SHALL NOT wake.

#### Scenario: Waking the citadel
- **WHEN** a skyship carrying a Magister docks at a dormant citadel and the commander uses the aetherstone with enough aether
- **THEN** the Magister joins the garrison, the ritual runs, and the citadel is woken for the commander's side

#### Scenario: No caster, no waking
- **WHEN** the commander uses the aetherstone with no caster in the garrison
- **THEN** the citadel stays dormant and the commander is told a caster is needed

### Requirement: The aether core
Inside the aether core the commander SHALL float: they do not fall, jump raises them and crouch lowers them.

#### Scenario: Floating in the aether core
- **WHEN** the commander steps into the aether core and holds jump, then crouch
- **THEN** they hang in the air without falling, rise while jump is held and sink while crouch is held

### Requirement: Steering from the throne hall
A woken citadel SHALL be steered from its throne hall's war table: pointing on the map sends it slowly there.

#### Scenario: Steering the citadel
- **WHEN** the commander at the throne hall's table selects the citadel and points at the map
- **THEN** the citadel heads there at its slow speed, with the commander aboard

### Requirement: The scrying floor
A woken citadel SHALL see far below it for its side, and its throne hall SHALL show the land beneath it on its scrying floor.

#### Scenario: Seeing from the citadel
- **WHEN** a citadel woken by a side drifts over an enemy camp
- **THEN** the side's picture gains the camp's buildings and the scrying floor's camera looks down on the land beneath the citadel

### Requirement: Drop-gates
A woken citadel SHALL drop its garrison through its drop-gates onto open ground beneath it.

#### Scenario: Through the drop-gates
- **WHEN** a woken citadel over land drops its garrison
- **THEN** its companies stand on the ground beneath it and the garrison is empty

### Requirement: Sky bombardment
A woken citadel with bombardment on SHALL strike enemy structures beneath it, paying aether for each strike.

#### Scenario: Bombarding from the sky
- **WHEN** a woken citadel with bombardment on hangs over an enemy building
- **THEN** the building takes damage and the side's aether goes down
