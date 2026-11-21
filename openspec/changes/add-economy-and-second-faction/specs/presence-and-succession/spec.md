# Spec Delta

## Purpose

The commander's presence on the field, and what happens when they fall.

## ADDED Requirements

### Requirement: Presence in battle
Companies of the commander's side within 50 m of the commander SHALL hold longer before breaking and strike harder.

#### Scenario: Holding with the commander
- **WHEN** a company loses half its men with the commander beside it
- **THEN** it does not break, where it would without the commander

### Requirement: Succession
The commander SHALL have health and be attacked by enemies; at no health the commander SHALL fall, and after a delay the side SHALL continue with a successor in the war room of the best succession anchor, with the sending stones silent for a handover period; a side with no successors or anchors left SHALL be defeated.

#### Scenario: The banner passes to the second
- **WHEN** the commander falls with a keep standing
- **THEN** after the delay the successor wakes at the keep's war table, the stones are silent for the handover, and then work again

#### Scenario: No one left to lead
- **WHEN** the commander falls with no successors left
- **THEN** the side is defeated
