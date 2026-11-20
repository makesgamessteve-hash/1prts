# Spec Delta

## Purpose

Lets one codebase and one data set build, run and be packaged as 1stPRTS on Windows, Linux, macOS, web desktop, mobile web and WebXR-capable browsers.

## ADDED Requirements

### Requirement: Native desktop builds
The project SHALL build from a single source tree into a native executable for Windows (x64), Linux (x64 and arm64) and macOS (arm64 and x64), using OpenGL 3.3 core profile features only.

#### Scenario: Build on each desktop platform
- **WHEN** the release build preset is configured and built on Windows, Linux or macOS with a supported compiler
- **THEN** the build produces a `1stprts` executable without errors
- **AND** the executable starts, opens a window and shows the front-end menu defined by the base game scripts

#### Scenario: OpenGL context unavailable
- **WHEN** the executable starts on a system that cannot create an OpenGL 3.3 core context
- **THEN** the game shows a readable error message naming the missing graphics requirement and exits with a non-zero status

### Requirement: Web build
The project SHALL build with Emscripten into a web bundle of an HTML page, a JavaScript loader, a WebAssembly binary and a separate data package, which runs in current desktop and mobile browsers that support WebGL 2.

#### Scenario: Web bundle runs in a browser
- **WHEN** the web release bundle is served over HTTP and opened in a WebGL 2 capable browser
- **THEN** the data package is downloaded with visible progress and the front-end menu appears

#### Scenario: Browser without WebGL 2
- **WHEN** the web bundle is opened in a browser without WebGL 2
- **THEN** the page shows a message that WebGL 2 is required instead of a blank screen

### Requirement: Same game on every platform
All platforms SHALL run the same base game from the same data content. Platform differences SHALL be limited to presentation defaults (resolution, draw distance, simulation detail radius) and the available input schemes.

#### Scenario: Same scenario result on desktop and web
- **WHEN** the Proving Ground elevator scenario is run on a desktop build and on the web build
- **THEN** both runs report the scenario as passed with the same final cell for the commander

### Requirement: Package layout
Every desktop release package SHALL contain the executable with a `data/` folder next to it, plus the license files. The web package SHALL contain the HTML, JavaScript, WebAssembly and data package files. No package SHALL require installation steps beyond extracting or serving it.

#### Scenario: Extract and run
- **WHEN** a desktop release package is extracted to an empty folder and the executable is started from there
- **THEN** the game finds its data and starts without any further setup

#### Scenario: Licenses included
- **WHEN** any release package is inspected
- **THEN** it contains the MIT license for code, the CC BY 4.0 license for art, the licensing guide and the third-party license files
