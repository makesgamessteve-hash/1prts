# Spec Delta

## Purpose

The Gloam's casters: the Glimmer-wright and the Mycel keeper.

## ADDED Requirements

### Requirement: The Glimmer-wright
The Glimmer-wright SHALL cast Glowflare (stuns enemies around the target, Durrow immune), Stonemeld (collapses or opens a gallery) and Crystal Sight (shows units in the area, also in tunnels).

#### Scenario: Glimmer-wright spells
- **WHEN** a Glimmer-wright casts Glowflare on an enemy company, Stonemeld on a gallery and Crystal Sight over a tunnel with enemies in it
- **THEN** the company stands stunned, the gallery collapses, and the enemies in the tunnel appear in the side's picture

### Requirement: The Mycel keeper
The Mycel keeper SHALL cast Spore Cloud (enemies in it strike weaker and move slower), Fungal Mend (heals the side's units in the area, twice as much underground) and Echo Call (rumoured sightings of every enemy within 300 m).

#### Scenario: Mycel keeper spells
- **WHEN** a Mycel keeper casts Spore Cloud on an enemy company, Fungal Mend on wounded Durrow and Echo Call
- **THEN** the enemy strikes weaker and moves slower, the Durrow heal, and far enemies show up as old sightings
