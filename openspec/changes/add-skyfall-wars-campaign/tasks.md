# Tasks: The Skyfall Wars campaign (M10)

## 1. Engine

- [x] 1.1 `prts.app.restart`, `prts.app.restart_pending`, the launcher's restart; C test `engine.restart_on_another_map` (spec *The next act*)

## 2. The campaign

- [x] 2.1 `game/campaign.lua`: acts, progress, unlocking, Continue; the Campaign screen; verify `campaign_menu.lua` (spec *The campaign screen*)
- [x] 2.2 Act missions: title, briefing, won and lost, pause menu entries; verify `campaign_acts.lua` (spec *An act is won*, *An act is lost*, *The next act*)

## 3. Act VIII

- [x] 3.1 `maps/skyfall_meres.lua`: Caldrith woken, the Custodians, the council; verify `custodians_wake.lua` (spec *The core is held*, *Sinking the citadels*)
- [x] 3.2 The other endings; verify `custodians_wake_endings.lua` (spec *Steering or sharing*)
