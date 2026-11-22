# Spec Delta

## Purpose

WebXR hand tracking.

## ADDED Requirements

### Requirement: Hands instead of controllers
With tracked hands and no controllers, a pinch SHALL act as the trigger of that hand, and a pinch of the left hand SHALL walk the commander (or aim a teleport).

#### Scenario: Pinching to walk and to use
- **WHEN** a player in VR with tracked hands pinches with the left hand and then with the right
- **THEN** the commander walks while the left hand pinches, and the right hand becomes the pointer hand
