# Spec Delta

## Purpose

How buildings rise in stages, and how timber burns.

## ADDED Requirements

### Requirement: Construction stages
A building site SHALL go through stages (staked out, foundations, frame and scaffolding, walls and roof, finished) that show as it is built, and the commander's presence SHALL speed building by a quarter.

#### Scenario: A barracks rises in stages
- **WHEN** serfs build a barracks
- **THEN** the site passes through each stage in order before it is finished

#### Scenario: Presence speeds building
- **WHEN** the same building is built once with the commander standing by and once without
- **THEN** it is finished about a quarter faster with the commander there

### Requirement: Fire
Timber buildings SHALL catch fire, burn, lose health, spread fire to timber neighbours and be put out by workers; stone buildings SHALL NOT burn.

#### Scenario: Firebrands burn a barracks
- **WHEN** Kobb firebrands attack a timber barracks with timber buildings next to it
- **THEN** the barracks burns and loses health and the fire spreads to a neighbour

#### Scenario: Serfs put the fire out
- **WHEN** a building of a side burns and the side has serfs nearby
- **THEN** serfs come and put the fire out

#### Scenario: Stone does not burn
- **WHEN** fire is set to a stone keep
- **THEN** it does not burn
