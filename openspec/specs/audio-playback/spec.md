# audio-playback Specification

## Purpose

Plays sound effects positioned in the 3D world and streamed music from data files, with volume control per category and sound muffled between interior cells.

## Requirements

### Requirement: Positional sound
The engine SHALL play WAV and OGG Vorbis sound effects at world positions or attached to entities. Volume SHALL fall off with distance and stereo panning SHALL follow the direction relative to the listener, which is the commander's head.

#### Scenario: Sound to the left
- **WHEN** a door to the commander's left plays its opening sound
- **THEN** the sound is louder in the left channel than in the right channel

### Requirement: Streamed music
Music tracks SHALL be streamed from OGG Vorbis files without loading the whole file up front. Scripts SHALL be able to start, stop and cross-fade tracks.

#### Scenario: Cross-fade
- **WHEN** a script cross-fades from the menu track to the Proving Ground track over 2 seconds
- **THEN** the first track fades out while the second fades in, with no gap or click

### Requirement: Volume buses
Sounds SHALL be grouped into at least the buses master, effects, voice, music and ambience. Each bus volume SHALL be adjustable in the options and SHALL be saved with the settings.

#### Scenario: Muting music
- **WHEN** the player sets the music volume to zero
- **THEN** music is silent while effects remain audible

### Requirement: Muffling between cells
Sounds playing in a different interior cell than the listener SHALL be quieter and duller when every portal between the two cells is closed, and SHALL become clear when a connecting portal opens.

#### Scenario: Door closes on a noisy room
- **WHEN** the forge room plays its hammering loop and the commander closes the door between the forge room and the corridor
- **THEN** the forge sound in the corridor becomes quieter and loses high frequencies

### Requirement: Browser audio unlock
On the web, audio SHALL start after the first user interaction as browsers require, without losing sounds that the game had requested to start before then.

#### Scenario: First click starts music
- **WHEN** the web build shows the menu and the player clicks or taps for the first time
- **THEN** the menu music starts playing

### Requirement: Pitch
A script SHALL be able to play a sound at a pitch between 0.5 and 2 (the rate it is played at); other values are an error.

#### Scenario: Pitch changes the playback rate
- **WHEN** a sound is played at pitch 2 and the same sound at pitch 1
- **THEN** the first one ends in half the time of the second
