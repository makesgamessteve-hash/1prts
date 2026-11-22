# Spec Delta

## Purpose

Ground that can be dug: holes in the terrain, and the space under it.

## ADDED Requirements

### Requirement: Holes in the terrain
Inside a terrain hole the terrain SHALL be neither drawn nor solid, whether the hole was made with the terrain or cut later.

#### Scenario: Down through a hole
- **WHEN** a hole is cut in the terrain above a gallery and the commander steps into it
- **THEN** they fall through where the ground was and land on the gallery's floor

### Requirement: Under the ground
More than 3 m under the terrain's surface the terrain SHALL NOT push bodies up, so galleries built there are walkable; the navigation grid SHALL keep units on the field above them.

#### Scenario: Walking a gallery under the field
- **WHEN** the commander walks a gallery 10 m under a field that soldiers cross above
- **THEN** the commander stays in the gallery and the soldiers walk on the field
