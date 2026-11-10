# PROJECT_STATUS — EARTH FRONTIER

**Ημερομηνία:** 2026-11-10  
**Version / branch:** `0.0.1-phase0` / `main` (git, 1 commit)  
**Milestone:** PHASE 0 — Architecture & Project Bootstrap (core complete; engine side unverified)

## Engine
- Στόχος: **Unreal Engine 5.4+** (TDR-001). Godot 4 μόνο ως τεκμηριωμένο fallback (TDR-002, δεν υπάρχει).
- Σε αυτό το περιβάλλον: **δεν υπάρχει UE5 ούτε Godot** → κανένα engine build/screenshot/gameplay δεν δοκιμάστηκε.

## Modules

| Module (spec §37) | Κατάσταση | Που |
|---|---|---|
| Core/Foundation (Json, Rng, Result) | IMPLEMENTED + TESTED | `EFWorldCore/Json.*, Rng.h, Result.h` |
| WorldClock & Simulation Director | IMPLEMENTED + TESTED (Tier R/G, catch-up) | `Simulation.cpp` |
| Entity Registry & Identity | IMPLEMENTED (string stable IDs, content-driven) | `Domain.h`, `ContentLoader.cpp` |
| Player/Character (domain) | IMPLEMENTED + TESTED | `WorldState.cpp` |
| Possession & Role/Seat Controller (domain) | IMPLEMENTED + TESTED | `WorldState.cpp` (Enter/Exit/SwitchSeat) |
| Interaction System (domain) | IMPLEMENTED + TESTED (NPC, cargo) | `WorldState.cpp` |
| Inventory & Equipment | PARTIAL (inventory + capacity; no equipment slots) | `Domain.h` |
| Vehicles Ground / Aviation / Naval (domain) | IMPLEMENTED (shared VehicleState/Definition; class rules) | `Domain.h`, `WorldState.cpp` |
| Vehicles — physics/handling/cameras | NOT STARTED (Phase 2–4, engine) | — |
| AI/Navigation | NOT STARTED (Phase 1/6) | — |
| Factions/Reputation | IMPLEMENTED + TESTED (ledger −100..100, bands) | `Reactions.cpp` |
| Economy/Infrastructure | IMPLEMENTED + TESTED (stock/shortage/price hint; weather→infra) | `Simulation.cpp`, `Reactions.cpp` |
| Mission & Campaign | IMPLEMENTED (objective graph Interact/TransportCargo, prerequisites, rewards once); Campaign = flags only | `Reactions.cpp` |
| Crew/Squad | NOT STARTED | — |
| Weather/Environment (domain) | IMPLEMENTED + TESTED (seeded Markov chain) | `Simulation.cpp` |
| Audio/Visual Presentation | NOT STARTED | — |
| UI | NOT STARTED; projections ready (`missionBoard`, `npcDialogue`, `news`, `debugSummary`) | `Reactions.cpp` |
| Save/Load | IMPLEMENTED + TESTED (atomic, backup, rollback, fallback, schema v1) | `SaveService.cpp`, `Snapshot.cpp` |
| QA/Debug/Telemetry | PARTIAL (`debugSummary`, journal lastN; no CPU timings) | `Reactions.cpp` |
| UE5 integration | **STUB / UNVERIFIED** (`UEFWorldSubsystem`, Build.cs, uproject, INIs) | `Source/EarthFrontier/` |

## Last verified build
- `cmake -S . -B build && cmake --build build && ctest --test-dir build` → **5/5 executables, 19/19 cases PASS** (gcc 13.3, `-fno-exceptions -fno-rtti`, 0 warnings with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`).
- `./build/ef_headless Content/Data/asterion_island_v1.json` → **10/10 acceptance steps PASS**, 392 journal events, 40 news items, snapshot byte-identical after save/load (`Docs/acceptance_run_phase0.log`).

## Known bugs / limitations
1. Economy balance placeholder: clinic & Pinewatch hit 0 stock within one game day (consumption 1/step). Tune in content, not code.
2. `RegionStabilityChanged` emitted every Tier-G step while recovering (hourly journal noise). Consider emitting only on ≥0.01 change.
3. Interaction radius uses POI centre (no NPC schedules/positions yet).
4. `EntityPoseChanged` reducer sets region by nearest POI — placeholder until region polygons/World Partition data exist.
5. UE glue never compiled: expect minor UHT/UBT fixes on first build (e.g. `Config=Game`, include order).

## Next concrete task (Phase 0 → Phase 1 gate)
1. **Χρήστης:** άνοιγμα `EarthFrontier.uproject` σε UE 5.4+, build `EarthFrontierEditor`, αποστολή Output Log (gate TDR-001 §5).
2. **Μόλις περάσει το gate:** Phase 1 — `L_Asterion_Bootstrap` (flat landscape + 6 POI markers σε μέτρα×100), `AEFPlayerPawn` FPS/TPS με Enhanced Input (IMC_OnFoot), `AEFNpcMarker` interact → `UEFWorldSubsystem::InteractNpc`, HUD widget που διαβάζει `GetClockText/GetCredits/GetNpcDialogue`, Save/Load menu → acceptance Phase 1 (start → interact → camera toggle → save → load).
3. **Ανεξάρτητα από engine:** tune consumption rates; add `MissionFailed` emission rule (time limit) + test; add `Docs/ARCHITECTURE.md` diagram export.
