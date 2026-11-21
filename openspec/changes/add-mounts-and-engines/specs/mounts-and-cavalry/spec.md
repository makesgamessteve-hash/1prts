# Spec Delta

## Purpose

The commander rides, and so do the Crown's knights: horses, mounted movement through the
commander's eyes, cavalry with a charge, and companies that ride with the commander.

## ADDED Requirements

### Requirement: Riding a horse
The commander SHALL mount a saddled horse by using it, SHALL then see from the saddle (a higher eye with the horse's head in view) and move at trot, canter and gallop speeds, and SHALL dismount with the mount action, leaving the horse beside them.

#### Scenario: Mounting and galloping
- **WHEN** the commander uses a horse at the stables and then gallops across the meadow
- **THEN** the eye is at saddle height and the commander covers ground at least twice as fast as on foot

#### Scenario: Dismounting
- **WHEN** the mounted commander presses the mount action
- **THEN** the commander stands on foot at walking speeds and a horse stands beside them

### Requirement: Cavalry
Stables SHALL train knights and outriders, and a mounted unit's first strike after riding at speed SHALL do double damage (a charge).

#### Scenario: Knights charge
- **WHEN** a squad of knights rides into men-at-arms at speed
- **THEN** their first strikes do double damage

### Requirement: Riding with the commander
A company SHALL follow the commander when ordered to ride with them (sending stone or horn), keeping close behind as the commander moves.

#### Scenario: Ride with your knights
- **WHEN** the mounted commander orders a company of knights to ride with them and gallops away
- **THEN** the knights follow and are close behind the commander when the commander stops
