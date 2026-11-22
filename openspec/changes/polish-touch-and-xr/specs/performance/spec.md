# Spec Delta

## Purpose

Performance on phones and standalone headsets.

## ADDED Requirements

### Requirement: Draw distance
Sprites and moving things farther than the draw distance SHALL not be drawn, and the fog SHALL close in before it.

#### Scenario: A shorter draw distance
- **WHEN** the draw distance is cut to 120 m over a skirmish with armies far apart
- **THEN** fewer sprites are drawn than with the full distance

### Requirement: Performance profiles
The game SHALL offer low, medium and high performance profiles and SHALL start on low on touch screens and in headsets.

#### Scenario: A phone starts on low
- **WHEN** the game first starts with the touch scheme active
- **THEN** the low profile is chosen and the draw distance is short
