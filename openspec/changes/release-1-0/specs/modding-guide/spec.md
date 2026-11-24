# Spec Delta

## Purpose

Mods start with the game through an entry point, and a guide with a working example shows how.

## ADDED Requirements

### Requirement: Mod entry points
The game SHALL run each enabled mod's entry point at boot, in load order, and SHALL carry on when one fails.

#### Scenario: A mod starts with the game
- **WHEN** the game starts with a mod that has `scripts/mods/<id>/init.lua`
- **THEN** its entry point has run before the first map starts

### Requirement: A documented example mod
The modding guide SHALL walk through an example mod that is tested in CI.

#### Scenario: The example mod works
- **WHEN** the game starts with the example mod on its map
- **THEN** its map plays as a match, with its own unit, its rule and its language
