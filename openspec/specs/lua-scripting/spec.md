# lua-scripting Specification

## Purpose

Runs all game-specific logic (boot flow, rules, definitions, interface content, interactions) as Lua scripts loaded from data, with a sandboxed, versioned and failure-tolerant engine API.

## Requirements

### Requirement: Script-driven boot
After the engine starts and mounts the data, it SHALL run `scripts/main.lua` from the virtual file system. The front-end menu, the available maps and what happens when a map starts SHALL be decided by scripts, not by engine code.

#### Scenario: Replacing the boot script
- **WHEN** a mod provides its own `scripts/main.lua` that shows a different menu
- **THEN** the game shows the mod's menu at start-up

### Requirement: Versioned engine API
Scripts SHALL access the engine only through the `prts` API table, which exposes its version as `prts.api_version`. Engine objects SHALL be passed to scripts as handles, never as memory addresses, and using a stale handle SHALL raise a script error instead of crashing.

#### Scenario: Stale entity handle
- **WHEN** a script stores an entity handle, the entity is destroyed, and the script then reads the entity's position
- **THEN** the call raises a stale-handle script error and the engine keeps running

### Requirement: Sandbox
Scripts SHALL NOT be able to run programs, access files outside the virtual file system, load native code or load precompiled bytecode. `require` SHALL load modules through the virtual file system.

#### Scenario: Blocked operating system access
- **WHEN** a script calls `os.execute`, `io.open` or `package.loadlib`
- **THEN** the call fails because the function is not available, and nothing outside the sandbox happens

#### Scenario: Module loading through the file system
- **WHEN** a script calls `require("game.rules")`
- **THEN** the engine loads `scripts/game/rules.lua` from the highest-priority mount that has it

### Requirement: Error containment
A runtime error in any script callback SHALL be caught at the engine boundary, logged with file, line and traceback, and shown in the developer console. The engine SHALL keep running, and the failing callback SHALL NOT stop other scripts from running.

#### Scenario: Error in an event handler
- **WHEN** a door's `use` handler raises an error while the player uses the door
- **THEN** the error and its traceback appear in the log and developer console, and the game keeps running at normal frame rate

### Requirement: Script time budgets
Each script entry point SHALL run under an execution budget. A script that exceeds the hard limit (such as an endless loop) SHALL be interrupted with an error, and the engine SHALL continue.

#### Scenario: Endless loop
- **WHEN** a script handler enters `while true do end`
- **THEN** the handler is interrupted within the hard limit, an error is logged, and the next frame is rendered

### Requirement: Deterministic script environment
Scripts running in the simulation SHALL get time only from the simulation clock and random numbers only from seeded simulation streams. Wall-clock time and unseeded random sources SHALL NOT be available to them.

#### Scenario: Same seed, same random numbers
- **WHEN** a scenario using `math.random` is run twice with the same seed
- **THEN** the scripts receive the same sequence of random numbers in both runs

### Requirement: Hot reload in developer mode
In developer mode on desktop, changing a Lua file under a mounted folder SHALL reload that module without restarting. Data stored on entities by scripts SHALL be kept, and handlers registered by name SHALL be replaced.

#### Scenario: Changing a door script while playing
- **WHEN** developer mode is on, the player is standing at a door, and the door's script file is edited to change its opening speed
- **THEN** the next use of the door uses the new speed, and the world state is otherwise unchanged

### Requirement: Developer console
Developer builds SHALL provide an in-game console that shows log output and script errors, runs Lua commands in the game's script environment, and can reload all scripts.

#### Scenario: Running a command
- **WHEN** the player opens the developer console and enters `prts.log.info(prts.api_version)`
- **THEN** the console prints the API version
