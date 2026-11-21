# Spec Delta

## Purpose

Great castles in the base game that the commander can walk through: curtain walls with
wall-walks, towers, gatehouses and the buildings of the wards, built from reusable building
blocks defined in Lua.

## ADDED Requirements

### Requirement: Concentric castle
The Proving Ground SHALL be Harrowmark, a concentric castle around the Great Keep with an inner and an outer curtain wall, towers along both, a gatehouse in each curtain, a great tower taller than the keep, and buildings in both wards.

#### Scenario: Approaching the castle
- **WHEN** a new Proving Ground session starts
- **THEN** the commander stands on the road before the outer gatehouse with the curtain walls and towers in view

### Requirement: Walkable walls and towers
Every curtain wall SHALL have a wall-walk with battlements on its outer edge, reachable from the ward by a stair, and the towers on the walls SHALL be enterable from the ward at ground level and from the wall-walks at wall-walk height. The great tower SHALL have a stair from its ground floor to its roof.

#### Scenario: Onto the wall-walk and into a tower
- **WHEN** the commander climbs a wall stair, steps onto the wall-walk and walks along it to a tower
- **THEN** they enter the tower on its floor at wall-walk height

#### Scenario: To the top of the great tower
- **WHEN** the commander enters the Raven Tower and follows its spiral stair upward
- **THEN** they reach the roof 36 m above the ward

### Requirement: Gatehouse portcullis
Each gatehouse SHALL have a portcullis that blocks its passage when lowered and is raised and lowered with a winch that the commander can use.

#### Scenario: Closing the gate
- **WHEN** the commander uses the King's Gate winch while the portcullis is up, and later walks at the gate from outside
- **THEN** the portcullis comes down and the commander cannot pass until it is raised again
