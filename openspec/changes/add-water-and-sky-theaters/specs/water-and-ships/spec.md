# Spec Delta

## Purpose

Water as a theater: the sea, ships that are moving frames with rooms and crews, harbors, naval
fighting and damage control.

## ADDED Requirements

### Requirement: Water
A map SHALL have a sea level; land units SHALL keep out of deep water, the commander SHALL wade slowly where the water is shallow enough, and ships SHALL keep to water deep enough for their draft.

#### Scenario: Wading at the shore
- **WHEN** the commander walks from the beach into the water
- **THEN** the commander slows down in the water, and soldiers ordered across the bay go around by land or stop at the shore

### Requirement: Ships
Ships SHALL sail where they are ordered and carry whoever stands on their decks, cogs and longships SHALL take companies aboard and land them on a shore, and ships SHALL fight with broadsides or rams.

#### Scenario: A cog lands troops
- **WHEN** a company embarks on a cog, the cog sails across the bay and lands them
- **THEN** the company stands ashore on the far side with its men

#### Scenario: A broadside
- **WHEN** a war galleon comes within range of an enemy longship
- **THEN** its guns fire and the longship is damaged

### Requirement: Damage control
A ship's rooms SHALL take the hits; damaged hull rooms SHALL let water in, the crew at the pumps SHALL bail it out, and a ship that fills SHALL sink.

#### Scenario: Holed below the waterline
- **WHEN** a galleon's hold is holed and its pump crew is lost
- **THEN** the ship floods, settles lower and sinks

#### Scenario: The pumps keep her afloat
- **WHEN** a galleon's hold is holed and its pump crew works
- **THEN** the water is held below the sinking point

### Requirement: Harbors and chart rooms
Harbors (Crown) and raft-yards (Wildborn) SHALL stand on a shore and launch ships, and a war galleon's chart room SHALL hold a war table that works as a naval command post.

#### Scenario: A ship from the harbor
- **WHEN** a side with a harbor orders a caravel
- **THEN** after the build time a caravel floats off the harbor's seaward side

#### Scenario: Orders from the chart room
- **WHEN** the commander points at a company on the galleon's chart-room table and then at a place
- **THEN** the company is ordered there, while the galleon sails
