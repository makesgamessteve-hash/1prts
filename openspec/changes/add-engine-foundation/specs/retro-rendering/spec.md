# Spec Delta

## Purpose

Draws the world in the 1stPRTS retro 3D style (low internal resolution, sharp pixels, vertex lighting, fog and optional palette effects) using meshes, sprites and voxels, with the look defined by data and shaders loaded from data.

## ADDED Requirements

### Requirement: Low internal resolution
The scene SHALL be rendered at a selectable internal resolution (presets 320×180, 480×270, 640×360, 960×540 and native, adjusted to the display's aspect ratio) and scaled to the output with nearest-neighbor sampling, using integer scaling when the output size allows it.

#### Scenario: Changing the preset
- **WHEN** the player selects the 320×180 preset in the video options
- **THEN** the scene is rendered at 320×180 (aspect-adjusted) and shown with sharp pixel edges, without restarting the game

### Requirement: Retro effects can be toggled
Nearest-neighbor texture filtering, vertex snapping, affine texture mapping, palette quantization with ordered dithering, distance fog and CRT screen effects SHALL each be switchable on or off at run time.

#### Scenario: Turning off dithering
- **WHEN** the player turns off palette quantization and dithering
- **THEN** the next frame is rendered without palette mapping or dither pattern

### Requirement: Shaders and palettes from data
All shader programs and color palettes SHALL be loaded from the data folder. A shader that fails to compile SHALL be reported with its file, line and the driver message, and the engine SHALL keep using a fallback shader.

#### Scenario: Mod changes the look
- **WHEN** a mod overrides the post-process shader file
- **THEN** the game renders with the mod's shader without the executable being rebuilt

#### Scenario: Broken shader
- **WHEN** a shader file contains a syntax error
- **THEN** the error names the shader file and line, affected objects are drawn with the fallback shader, and the game keeps running

### Requirement: Mesh, sprite and voxel content
The renderer SHALL display glTF 2.0 models (including their textures, vertex colors and node hierarchy), billboard sprites with 8 view directions and animation frames, and MagicaVoxel `.vox` models.

#### Scenario: Directional sprite
- **WHEN** the commander walks in a circle around a standing person sprite
- **THEN** the sprite shows its front, side and back frames according to the viewing angle

#### Scenario: Voxel prop
- **WHEN** the Proving Ground loads a `.vox` crate
- **THEN** the crate is drawn as a solid, lit object with its voxel colors

### Requirement: Retro lighting and fog
Lighting SHALL be computed per vertex from a directional light, ambient light, a limited number of point lights and baked vertex colors. Distance fog SHALL use per-map fog colors and ranges.

#### Scenario: Dusk lighting
- **WHEN** the Proving Ground's lighting script sets the dusk preset
- **THEN** the sun color, ambient color and fog color change to the preset's values on the next frame

### Requirement: Wide depth range
The renderer SHALL draw objects from 0.05 m to at least 100 km from the viewer in the same view without visible depth fighting between surfaces at least 0.5% of their distance apart.

#### Scenario: Slate and horizon in one view
- **WHEN** the commander holds a placeholder slate in front of them while the horizon is 20 km away
- **THEN** both the slate and distant terrain are drawn with correct occlusion and no flicker

### Requirement: Interior visibility through portals
Interiors SHALL be divided into cells connected by portals. From inside a cell, only cells visible through open portals SHALL be drawn. From outside, an interior SHALL be drawn only through its open portals and windows.

#### Scenario: Closed door hides a room
- **WHEN** the commander stands in the HQ lobby facing a closed door to a side room
- **THEN** the side room's contents are not drawn, and opening the door makes them visible

### Requirement: In-world screens
The renderer SHALL be able to draw additional views and user-interface content into textures shown on surfaces in the world, such as monitors and the map table, at their own low resolution and update rate.

#### Scenario: Live monitor
- **WHEN** the commander looks at the placeholder map table in the Command Center
- **THEN** its screen shows a live, animated view rendered into the table's surface with the CRT effect

### Requirement: Stereo-ready views
The renderer SHALL render a list of views per frame, each with its own viewport, view and projection, so that the same scene can be drawn once for a flat screen or once per eye for XR.

#### Scenario: Two views
- **WHEN** an XR session provides two eye views
- **THEN** the scene is drawn into both views in the same frame using each view's own matrices
