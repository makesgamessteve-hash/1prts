# Spec Delta

## Purpose

Measuring how the game plays, the same way every time, and catching changes that break the
ways through a map or make loading slow.

## ADDED Requirements

### Requirement: Fun audit
The project SHALL have an audit that plays a map with bots of different skill (random input, a human proxy with human reaction times and aim, a frame-perfect oracle, a one-button spammer), measures the commander's feel with probes, and reports the six pillar scores and a composite fun score in a fixed format.

#### Scenario: Auditing a build
- **WHEN** the fun audit runs on a build
- **THEN** it prints the pillar scores, the composite score, the feedback channels per event and any blocked ways, and writes them as JSON

### Requirement: Ways through a map stay open
The test suite SHALL include the audit's survey, in which the oracle walks every way of the map's route graph there and back, and it SHALL fail, naming the way, when one cannot be walked.

#### Scenario: A way through the map is broken
- **WHEN** a change blocks a way the route graph says can be walked
- **THEN** the survey fails and names the blocked way

### Requirement: Loading Harrowmark quickly
Starting a map from the menu SHALL give the commander control within 3 s on a desktop release build, and the faster light bake SHALL light every vertex exactly as before.

#### Scenario: Same light, less waiting
- **WHEN** Harrowmark is started from the menu in a release build
- **THEN** the commander can move within 3 s, and the golden images match their references
