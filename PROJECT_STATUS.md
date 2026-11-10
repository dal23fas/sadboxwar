# PROJECT_STATUS — EARTH FRONTIER

**Ημερομηνία:** 2026-11-10  
**Version / branch:** `0.0.3-all-classes` / `main`  
**Milestone:** Phase 0 complete (core) · Phase 1 + vehicle/realism prototype in Godot (unverified) · supplement integrated

## Engine
- **Godot 4.3+** (TDR-002). UE5 glue αφαιρέθηκε από το branch (history: `bb3b30c`).
- Εδώ: κανένα Godot, κανένα godot-cpp → ό,τι είναι GDScript/GDExtension **δεν εκτελέστηκε**. Ο C++ πυρήνας, ο TCP bridge και ο HTML viewer **εκτελέστηκαν και πέρασαν**.

## Modules

| Module | Κατάσταση | Που |
|---|---|---|
| Core: Json/Rng/Events/Journal | IMPLEMENTED + TESTED | `EFWorldCore` |
| WorldState commands/reducers/reactions | IMPLEMENTED + TESTED | `WorldState.cpp`, `Reactions.cpp` |
| Simulation Tier R/G, catch-up | IMPLEMENTED + TESTED | `Simulation.cpp` |
| Missions / Economy / Reputation / News / Weather→Infra | IMPLEMENTED + TESTED | `Reactions.cpp` |
| Save/Load (atomic, backup, rollback, fallback) | IMPLEMENTED + TESTED | `SaveService.cpp`, `Snapshot.cpp` |
| **JsonApi** (single host surface) | IMPLEMENTED + TESTED | `JsonApi.cpp`, `test_json_api` |
| **RealismProfileManager** (presets/overrides/reset/settings file) | IMPLEMENTED + TESTED (T10, T16, T11-settings, §14 invariant) | `RealismProfiles.cpp`, `test_realism` |
| **Submarine class** + content | IMPLEMENTED + TESTED (port rule, Harbor Link eligibility) | `Domain.cpp`, content |
| ef_server (TCP bridge) | IMPLEMENTED + TESTED (Python smoke incl. reconnect) | `Tools/ef_server` |
| HTML world viewer | IMPLEMENTED + TESTED (headless Chromium, screenshot) | `Tools/ef_viewer`, `Docs/viewer_acceptance_phase0b.png` |
| GDExtension wrapper | **STUB / UNVERIFIED** | `Source/EFGodot` |
| Godot: World bridge (ext or TCP), input map, bootstrap scene (land/sea/runway/quay), FPS/TPS player, NPC interact/dialogue, mission accept, vehicle enter/exit, ONE kinematic controller for wheeled / tracked (pivot turn) / helicopter / **fixed-wing (throttle, stall, bank-to-turn, auto-trim)** / boat / submarine (dive/surface), cargo load/deliver, HUD, save/load, debug overlay, Realism settings panel | **PROTOTYPE / UNVERIFIED** | `godot/scripts/*.gd` |
| Vehicle visuals: procedural fictional silhouettes per class with spinning rotors/props/radar and wheels; `.glb` override by definition_id | **PROTOTYPE / UNVERIFIED** | `godot/scripts/vehicle_visuals.gd`, `Docs/ASSET_PIPELINE.md` |
| Vehicle physics (real rigid-body, terrain, buoyancy), AI, interiors, audio, localization, Web export | NOT STARTED | — |

## Last verified build (this environment)
- `ctest` → **7/7 suites, 29/29 cases PASS**, 0 warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, `-fno-exceptions -fno-rtti`).
- `ef_headless` → **10/10 PASS** (`Docs/acceptance_run_phase0.log`).
- `ef_server` + `Tools/ef_server/test_client.py` → **PASS** (tick 30, save/load, reconnect).
- `Tools/ef_viewer/check_viewer.mjs` → **PASS**, 0 page errors (`Docs/viewer_acceptance_phase0b.png`).

## Known bugs / limitations
1. Godot scripts γράφτηκαν χωρίς editor: περιμένω μικρά parse/API λάθη στο πρώτο άνοιγμα (στείλε το Output panel).
2. Οχήματα = kinematic placeholders (CharacterBody3D), όχι RigidBody/VehicleBody3D. Έδαφος επίπεδο, θάλασσα επίπεδη πλάκα δυτικά του x=-400. Μοντέλα = procedural σιλουέτες μέχρι να μπουν .glb (ASSET_PIPELINE.md).
3. Economy balance: κλινική/Pinewatch αδειάζουν σε ~1 ημέρα· η δράση των §8 formulas είναι ορατή αλλά αυστηρή.
4. Hourly `RegionStabilityChanged` θόρυβος στο journal κατά το recovery.
5. Web export του GDExtension απαιτεί emscripten και αντικατάσταση του file-based `FileSaveStorage` (IndexedDB/user://) — Phase 8.
6. `interact` και `yaw_right` μοιράζονται το E (διαφορετικό context· rebinding UI Phase 7).

## Next concrete task
1. **Χρήστης (gate TDR-002 §4):** `cmake --build` → `./build/ef_server` → Godot 4.3+ Import `godot/project.godot` → Play. Στείλε το Output panel (ακόμη κι αν είναι μόνο parse errors) και, αν φτάσεις, το `Saved/quick.efsave`.
2. **Εγώ, μετά το log:** διόρθωση GDScript, μετά Phase 2 κυρίως: VehicleBody3D για wheeled/tracked με presets, terrain heightmap, garage/repair UI, `MissionFailed` κανόνας.
3. **Ανεξάρτητα:** tune consumption rates· AI vehicle input provider για το ίδιο `vehicle.gd` (Phase 6 προετοιμασία).
