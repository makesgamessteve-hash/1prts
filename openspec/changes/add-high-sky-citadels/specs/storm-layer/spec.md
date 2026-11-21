# Spec Delta

## Purpose

The storm layer: the cloud deck between the sky and the high sky, with poor sight, winds and
lightning.

## ADDED Requirements

### Requirement: Sight in the storm
Inside the storm band sight SHALL drop to a few dozen metres both ways, and the commander inside it SHALL see the fog close in.

#### Scenario: Inside the storm
- **WHEN** an enemy flyer circles inside the storm band 80 m from a side's watchtower and the commander's skyship climbs into the band
- **THEN** the side does not see the flyer, and the fog around the commander closes to a few dozen metres until they climb out

### Requirement: Storm winds
Flyers and skyships inside the storm band SHALL drift with the map's wind.

#### Scenario: Carried by the wind
- **WHEN** a skyship hangs inside the storm band with no orders
- **THEN** it drifts downwind

### Requirement: Lightning
Lightning SHALL strike flyers inside the storm band from time to time.

#### Scenario: Struck by lightning
- **WHEN** a wyvern circles inside the storm band for a minute
- **THEN** lightning strikes it at least once and it loses health
