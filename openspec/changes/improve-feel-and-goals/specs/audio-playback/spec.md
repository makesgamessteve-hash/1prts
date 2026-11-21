# Spec Delta

## Purpose

Sounds that repeat often (footsteps) vary a little so they do not sound mechanical.

## ADDED Requirements

### Requirement: Pitch
A script SHALL be able to play a sound at a pitch between 0.5 and 2 (the rate it is played at); other values are an error.

#### Scenario: Pitch changes the playback rate
- **WHEN** a sound is played at pitch 2 and the same sound at pitch 1
- **THEN** the first one ends in half the time of the second
