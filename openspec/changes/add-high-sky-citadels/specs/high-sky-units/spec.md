# Spec Delta

## Purpose

The tier-6 flyers that reach the high sky: the Crown's sky galleon and the Wildborn's elder drake.

## ADDED Requirements

### Requirement: Tier six
Sky galleons and elder drakes SHALL be trained only by a side at tier 6.

#### Scenario: Only at tier six
- **WHEN** a side below tier 6 orders a sky galleon or an elder drake, and then reaches tier 6 and orders again
- **THEN** the first order is refused and the second is taken

### Requirement: The sky galleon
The sky galleon SHALL fly at any height up to the high sky with whoever stands on it, and its guns SHALL fire at enemy flyers and structures in range.

#### Scenario: The sky galleon's guns
- **WHEN** a sky galleon hangs within gun range of enemy wyverns
- **THEN** its guns fire and the wyverns lose health

### Requirement: The elder drake
The elder drake SHALL breathe fire that sets timber alight, and SHALL board a citadel it reaches, putting its warriors into the garrison.

#### Scenario: Drakes board a citadel
- **WHEN** elder drakes are ordered at a citadel
- **THEN** they climb to its height and their warriors join its garrison
