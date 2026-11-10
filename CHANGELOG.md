# CHANGELOG

## 0.0.1-phase0 — 2026-11-10

### Added
- `Docs/TDR-001-engine-decision.md`: UE5 selected as target engine (provisional gate), engine-neutral C++ core strategy.
- `Source/EFWorldCore`: authoritative world state — JSON, deterministic RNG streams, event envelope + journal with dedup,
  domain states, commands with validation, reducers, derived reactions (missions, rewards, infrastructure, news),
  multi-rate simulation (Tier R economy, Tier G weather/stability, bounded catch-up), content loader, snapshot,
  SaveService with atomic write / backup / rollback / seated fallback.
- `Content/Data/asterion_island_v1.json`: Asterion Island content — 6 POIs, 5 NPCs, 3 vehicles, 2 infrastructures,
  3 economy nodes, 3 factions, 5 interlinked missions, state-driven dialogue.
- `Tests/`: 5 CTest executables / 19 cases covering CORE-01, ROLE-01..03, SAVE-01/02, MISSION-01, ECON-01, REPU-01,
  NEWS-01, WORLD-01/02, EVT-01, WS-04/06/09/10.
- `Tools/ef_headless`: runs the 10-step vertical-slice acceptance script in data form; log in `Docs/acceptance_run_phase0.log`.
- UE5 glue (UNVERIFIED): `.uproject`, Build/Target rules, `UEFWorldSubsystem` (commands + projections + tick driver), Config INIs.

### Known limitations
- No 3D presentation, input, cameras, physics yet (Phase 1+). Nothing engine-side compiled in this environment.
- Economy balance: clinic/hamlet consumption (1 item / 10 min) drains stock within a game day — placeholder values.
- `MissionFailed`, `RegionControlChanged` reducers exist but nothing emits them yet.
