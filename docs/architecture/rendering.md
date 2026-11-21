# Rendering

## 1. Graphics API baseline

| Target | API | Shading language |
|---|---|---|
| Windows, Linux | OpenGL 3.3 core | GLSL 330 |
| macOS | OpenGL 4.1 core (forward-compatible), using only 3.3 features | GLSL 330 |
| Web, WebXR | WebGL 2 (OpenGL ES 3.0) | GLSL ES 3.00 |

- The renderer uses only the **features common to GL 3.3 core and GLES 3.0**: VAOs, instancing,
  uniform buffers, multiple render targets, float textures where available, and sRGB framebuffers.
  It avoids geometry and tessellation shaders, compute shaders, clip control and
  `glPolygonMode`.
- **Shaders are assets** in `data/*/shaders/`, written once in a shared dialect. The engine puts
  a per-platform prelude at the top (`#version`, precision qualifiers, compatibility macros).
  Modders can change the whole look without recompiling.
- **macOS note**: Apple has deprecated OpenGL, but it still works. All GL calls stay inside
  `gfx/` so that another backend (for example through SDL_GPU) could be added later. That is not
  planned.

## 2. Views and stereo

The renderer always draws a **list of views**. Each view has a viewport, a view matrix, a
projection and a target framebuffer:
- Flat screen: 1 view.
- WebXR: 2 views (one per eye), drawn into the `XRWebGLLayer` framebuffer that the XR runtime
  provides, with view and projection matrices from `XRFrame.getViewerPose()`.
- In-world surfaces (scrying pools and mirrors showing far-away places): extra **offscreen views** that render into
  textures at low resolution, drawn before the main views and capped per frame.

Because stereo is in the design from the start, XR does not need a separate renderer.

## 3. Pass structure (per view)

```
1. Shadow-free scene, depth-layered (far → near), into the low-res scene target:
     a. Sky layer            gradient dome, stars, moons, sun, distant citadels (no depth)
     b. Far layer            far terrain chunks, impostors, ocean   (own depth range, then clear depth)
     c. Near layer           near terrain, structures, units, sprites (own depth range, then clear depth)
     d. Interior/instrument  current cell + portal-visible cells, held items (field map, spyglass)
2. Transparent and effects  sprites (smoke, fire), illusions (war table light lines), particles
3. Retro post-process       palette quantization + ordered dither (optional), fog already applied
4. Upscale                  nearest-neighbor blit to the window or XR framebuffer (integer scale when possible)
5. Overlay                  minimal HUD, touch controls (flat screen only), developer console
```

**Depth layers** replace a single huge depth range. WebGL 2 has no clip control, so reversed-Z
gives little benefit there. Splitting the scene into layers by distance keeps depth precision
good from 0.05 m (the field map in your hand) to tens of kilometers (views from the high sky).

## 4. Retro look implementation

| Effect | How |
|---|---|
| Low internal resolution | The scene renders into an offscreen target at the preset size (480 × 270 by default) |
| Sharp pixels | Nearest-neighbor sampling for textures and for the upscale |
| Vertex wobble | Vertex shader snaps clip-space positions to the internal-resolution grid (off in XR) |
| Affine warping | Optional `noperspective` texture coordinates (off by default) |
| Gouraud lighting | Per-vertex directional, ambient and point lights, plus baked vertex color |
| Fog | Per-vertex linear or exponential fog, colored per theater and time of day |
| Palette + dither | Post pass: a 3D lookup texture maps colors to the theater palette, and a Bayer matrix adds dithering |
| Magic surfaces | Display materials apply a shimmer, ripple and glow shader to render-target textures (war table, scrying pools) |

## 5. Geometry types

- **Meshes**: glTF 2.0 (`.glb`/`.gltf`), using a subset: positions, normals, UVs, vertex colors,
  materials (base color texture plus factors), node hierarchy, and named **marker nodes** (seats,
  doors, posts, portals, frame volumes, light points). Rigid parts can be animated (doors,
  lift platforms, trebuchet arms, and the segmented bodies of giants, wyverns and the behemoth).
  Skinning is not needed: people are sprites and large creatures are rigid-part models.
- **Terrain**: chunked heightfield (4 m spacing by default), with geometric LOD per chunk,
  skirts to hide seams, and splat texturing.
- **Sprites**: camera-facing billboards. People use **8-direction** sheets, and the frame is
  chosen from the angle between view and facing. Sprites are batched by atlas, and alpha-tested
  or additive.
- **Voxels**: `.vox` models are greedy-meshed into normal meshes when loaded (with vertex AO
  baked in), then drawn like any other mesh.
- **Deck plans** (from M3): compiled into kit-piece instances, then merged into per-cell static
  meshes.
- **Illusions**: the war table's map and the marking staff's placement outlines are line and
  thin-quad geometry with additive blending and a soft shimmer.

## 6. Visibility and LOD

- **Frustum culling** per frame-tree subtree, with bounding spheres in camera-relative space.
- **Portal culling** for interiors: start from the camera's current cell and go through open
  portals, narrowing the screen rectangle at each step. When outdoors, a building's interior
  cells are drawn only if they can be seen through an open portal or a window.
- **Underground and under water**: tunnels and caverns are portal-culled like interiors, and
  water volumes switch to a short fog distance and a blue-green tint, so both theaters stay cheap
  to draw.
- **Distance LOD** for meshes (2–3 levels plus an impostor) and terrain chunks. LOD bias is
  set per platform.
- **Budgets** (web desktop reference): ≤ 1,500 draw calls, ≤ 1.5 M triangles per view,
  ≤ 4 offscreen screen views per frame.

## 7. UI rendering

- An **immediate-mode UI** module draws into either the screen overlay or a **world-space
  surface** (a render target mapped onto a scrying pool, the field map or the war table).
- Input reaches world-space UI by **ray casting** from the commander's eyes (mouse/gamepad),
  the touch point, or the XR controller ray onto the surface's UV space.
- Text uses **bitmap fonts** (PNG atlas plus a metrics file) from `data/`.

## 8. WebXR frame flow

- **Outside XR**, SDL3 drives `SDL_AppIterate` from the browser's `requestAnimationFrame`.
- **Inside an immersive session**, the browser stops calling the window's
  `requestAnimationFrame` and calls the **XR session's** frame callback instead. The JS glue
  in `web/` takes this over. While the session is active it calls the engine's frame function
  from `XRSession.requestAnimationFrame`, passes in the viewer pose, eye views and controller
  poses, and binds the XR layer framebuffer. When the session ends, control goes back to SDL3's
  loop.
- The engine's frame function is shared, and only the **frame driver** differs, so the
  simulation, scripts and audio behave the same in and out of XR.
- **Head and body** (`engine/src/app/engine_xr.c`): the reference space (the play area) is
  anchored at the commander's feet and turned by the play area's yaw. Head and hand poses are
  frame input like mouse look; each tick reads them as the commander's XR input (recorded in
  replays), and the body follows the head across the floor until a wall stops it. At render time
  the head is swept out from the body's centre against the collision world, and every eye is
  shifted by the same push-back, so a head leaned into a wall stops 12 cm short of its surface.
- **XR presentation**: each controller's pointer ray is drawn as a line into both eyes (not
  into in-world surfaces); the comfort vignette darkens the view's edges in the upscale pass,
  centred on each eye's optical axis; and VR uses its own video settings (by default no vertex
  snapping, dithering, affine textures or crosshair, and the `hd` preset), so the flat screen's
  retro look and golden images are unchanged.
