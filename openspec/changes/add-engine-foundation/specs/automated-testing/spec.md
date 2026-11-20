# Spec Delta

## Purpose

Provides the test infrastructure and quality gates that keep the engine and the base game correct on every platform: headless running, unit and scenario test runners, coverage gates, golden images and browser checks.

## ADDED Requirements

### Requirement: Headless mode
The executable SHALL run without a window, GPU or audio device when started in headless mode, using null rendering and audio back ends, a fixed seed and an isolated user data folder, and SHALL simulate as fast as possible.

#### Scenario: Headless on a server
- **WHEN** the game is started in headless mode on a machine without a display
- **THEN** it loads the Proving Ground, runs the requested ticks and exits with status 0

### Requirement: Scenario test runner
The engine SHALL run automated game tests written as Lua scenario scripts that drive the game through the same actions as real input and check world state. Each scenario SHALL report pass or fail with a reason, the runner SHALL produce a machine-readable report, and SHALL exit non-zero if any scenario fails.

#### Scenario: Failing scenario
- **WHEN** a scenario expects the commander to be in the war room but they are still in the great hall when it times out
- **THEN** the scenario is reported as failed with the expected and actual cell, and the runner exits non-zero

#### Scenario: Report for CI
- **WHEN** the scenario runner finishes
- **THEN** it writes a JUnit-compatible XML report listing every scenario and its result

### Requirement: Lua unit test runner
The engine SHALL run Lua unit tests found in the mounted `scripts/tests/` folders, with assertions, grouped test cases, and the same reporting and exit status rules as the scenario runner.

#### Scenario: Running base game tests
- **WHEN** the Lua test runner is started for the base game
- **THEN** every test file under `data/base/scripts/tests/` is run and the results are reported

### Requirement: Coverage gate
Continuous integration SHALL measure line coverage for the engine's C code and for the base game's Lua scripts (excluding test scripts), and SHALL fail if either is below 95%. Excluded files SHALL be listed in one reviewed file, each with a reason.

#### Scenario: Coverage drops
- **WHEN** a change makes engine C line coverage 94.8%
- **THEN** the coverage job fails and reports the coverage per file

#### Scenario: Lua coverage report
- **WHEN** the engine runs tests in Lua coverage mode
- **THEN** it writes an lcov-format report covering every executable line of every loaded base game script

### Requirement: Determinism check
The test suite SHALL include checks that run scenarios twice and replay recorded sessions, and SHALL fail if any per-tick state hash differs.

#### Scenario: Non-deterministic code is caught
- **WHEN** a script reads an unseeded random source during the simulation
- **THEN** the determinism check fails and names the first tick where the hashes differ

### Requirement: Render golden images
The test suite SHALL render fixed scenes from fixed camera positions with the real graphics back end and compare them with stored reference images, failing if more than 0.5% of pixels differ by more than 8 levels in any color channel. Reference images SHALL change only through an explicit update command.

#### Scenario: Visual regression
- **WHEN** a change makes the fog color in the dusk scene noticeably different
- **THEN** the golden test for that scene fails and keeps a difference image as a CI artifact

### Requirement: Browser end-to-end checks
The test suite SHALL run the web build in a real browser engine and check that it boots to the menu, responds to keyboard input, shows touch controls on an emulated touch device, and enters and leaves an XR session with an emulated XR device.

#### Scenario: Emulated phone
- **WHEN** the browser check opens the web build with a touch-device profile
- **THEN** the touch controls are visible and a drag on the right half of the screen turns the view

### Requirement: Continuous integration on all platforms
Every pull request SHALL be built and tested on Windows, Linux, macOS and the web build. Unit, Lua and scenario tests SHALL run on every desktop platform. Merging SHALL be blocked while any job fails.

#### Scenario: Windows-only failure
- **WHEN** a change breaks a test only on Windows
- **THEN** the Windows job fails and the pull request cannot be merged
