# EARTH FRONTIER — ROADMAP, VERTICAL SLICE & ACCEPTANCE

## 1. Project delivery philosophy

**Priority: a playable, persistent, connected game before scale.** Do not begin by creating an entire planet or an exhaustive list of vehicles. Build one fictional island, one player, one car, one helicopter and one boat with shared missions; validate the core before expansion. Use **real project files** with source control and tests.

## 2. Phase dependency map

```mermaid
flowchart LR
  P0[0: Bootstrap & data contracts] --> P1[1: Avatar + interactions]
  P1 --> P2[2: Ground vehicle + possession]
  P2 --> P3[3: Helicopter + air transport]
  P3 --> P4[4: Boat + harbor]
  P4 --> P5[5: Missions + economy + reputation]
  P5 --> VS[VERTICAL SLICE ACCEPTED]
  VS --> P6[6: Factions + world AI]
  P6 --> P7[7: Campaign + roles + interiors]
  P7 --> P8[8: Content + polish + release]
```

The architecture already reserves contracts for future modules, but no unused modules need artificial full implementations. Avoid building complex systems before demonstrating an end-to-end mission loop.

## 3. PHASE 0 — Bootstrap

**Deliverables:** engine choice decision record; empty runnable project; standard folders; source control; input framework; world state structs; event journal interface; schema version; basic automated tests; test map and launch instructions.

**Acceptance:** game opens into minimal map, world clock ticks, app logs contain no critical errors, unit test of event deduplication passes (if executable test support available).

## 4. PHASE 1 — Playable character

**Deliverables:** walk/run, camera FPS/TPS, interaction prompts, one NPC dialogue, one terminal, basic HUD, pause/settings, save/load, zone streaming prototype.

**Acceptance:** start new → interact → change camera → save → load without losing player state; camera does not duplicate player/mission data.

## 5. PHASE 2 — Ground mobility

**Deliverables:** one fictional off-road vehicle, shared possession interface, drive/exit, vehicle camera, condition resource, basic repair/garage interface, one transport task.

**Acceptance:** enter car, drive through streaming boundary, exit, resume walking, complete NPC interaction and save/load car position.

## 6. PHASE 3 — Helicopter / initial aviation

**Deliverables:** helicopter model or licensed placeholder, assisted flight controls, landing/boarding, cockpit/chase camera, weather cues, 1 short relief transport mission, simple AI traffic.

**Acceptance:** player boards, transports game cargo to marked destination, lands, disembarks, objective triggers once and cargo disappears exactly once. Extend to fixed-wing only after helicopter lifecycle stable.

## 7. PHASE 4 — Naval

**Deliverables:** small boat, water surface, playable wheel/bridge station, navigation HUD, dock interaction, port services, passenger or supply mission, seat continuity.

**Acceptance:** board at port → navigate to destination → deliver → disembark → return to FPS/TPS, same mission/world state throughout. Weather affects ambiance and allowed activity via game rules.

## 8. PHASE 5 — Systems integration

**Deliverables:** global clock, world event bus, economy node stocks, two primary factions and one neutral organization, local reputation, 3–5 multi-role missions, world news.

**Acceptance:** 2+ distinct vehicle roles affect the *same* economy node; mission completion once; news shows committed event; save/load preserves credits, region stock, reputation and vehicle state.

## 9. VERTICAL SLICE — ASTERION PLAYABLE DEMO

**Map scope:** fictional Asterion Island: Port Lyria (town), Harbor West (port), Windfield (airfield), Beacon Clinic (aid destination), Depot South (vehicle garage), Pinewatch (remote hamlet). Map scale intentionally modest, dense and traversable, not entire Earth.

**Assets:** 1 avatar with FPS/TPS animations; 1 off-road truck; 1 helicopter; 1 boat; 1 optional fictional tracked vehicle as a later slice extension; 5 NPC archetypes; low/mid-detail environment with readable signs and transitions.

**Game state:** player ID; owned/available vehicles; current role; local credits; 2 factions + 1 neutral; 2 economy nodes; 4 items; 3–5 mission definitions; world time/weather; 1 dynamic infrastructure event; persistent journal.

**Five target missions:**

1. **First Contact** — speak with mayor, accept first task, learn inventory/maps.
2. **Road Supplies** — drive civil supplies between two fictional facilities.
3. **Sky Relief** — board helicopter and complete a transport mission.
4. **Harbor Link** — board boat and visit a separated coastal location.
5. **Community Update** — return to a town NPC and see a new dialogue/news generated from accumulated state.

**Optional encounter:** game-balanced fictional threat event which can be resolved through a story choice or non-graphic minigame; not needed to validate weapons/combat systems in the first slice.

## 10. Vertical slice acceptance script

| Step | Player action | Expected persistent outcome |
|---|---|---|
| 1 | Start new game in Port Lyria | Player profile and world tick created |
| 2 | Talk to Mayor Elara Venn | Mission ID accepted |
| 3 | Enter and drive truck | Same player ID and mission |
| 4 | Deliver goods | Economy stock changes exactly once |
| 5 | Walk to Windfield and board helicopter | Role/camera changes, world stable |
| 6 | Complete sky relief objective | Updated trust/reward |
| 7 | Travel to Harbor West, board boat | Boat movement works in same save |
| 8 | Return to town and read news | News links to exact committed events |
| 9 | Save/quit/load | All changes intact; no duplicate cargo/reward |
| 10 | Advance world time | Offscreen aggregate systems update predictably |

All above must be performed without editing code while playing. For tests that cannot run in automated environment, list the manual steps as **not yet verified**, not as passed.

## 11. PHASE 6 — Living world expansion

**Deliverables:** regional LOD simulation, factions and political relations, civic stability, world events, AI civilian schedules, infrastructure changes, local pricing.

**Acceptance:** an off-screen event creates a mission and news item; player can alter the state, save/load and observe continuing simulation; local NPC count no longer scales linearly with global map size.

## 12. PHASE 7 — More careers/campaign

**Deliverables:** fixed-wing flight, tracked vehicle gameplay, expanded ships, squad companions, command overview mode, branch-driven campaign, additional interiors, larger world chunks.

**Acceptance:** changing role does not reset faction or mission state, branching is deterministic per save, story reacts to actual world facts, new entities follow same interfaces.

## 13. PHASE 8 — Polish

**Deliverables:** polished models/materials, animation transitions, varied sound, environmental storytelling, Greek/English localization, accessibility, optimization, crash handling, save migration, installation guide.

**Acceptance:** full smoke test, regression suite, performance capture on specified hardware, no critical blockers, clean build packaging and documented known limitations.

## 14. Technical risks and mitigation

| Risk | Impact | Mitigation |
|---|---|---|
| Over-ambitious world scale | Unplayable / stalled | One island and evidence-based streaming expansion |
| Vehicle code duplication | Hard to maintain | Shared Vehicle / Possession / Seat interfaces |
| UI/world data drift | False progress | Authoritative services and read-only projections |
| AI CPU overload | Low FPS | 3-tier simulation LOD |
| Save corruption | Lost progress | Versioning, validation, atomic writes, backups |
| Engine tool unavailable | No executable build | Explicit audit, scoped alternative, honest status |
| Too many modes initially | Unfinished gameplay | Transport-first MVP, then advanced roles |
| Scope/IP problems | Distribution risks | Original fictional content and licensed assets |

## 15. Required status format for every delivery

```text
BUILD / MILESTONE:
ENGINE + VERSION:
FILES CREATED/CHANGED:
IMPLEMENTED:
PARTIAL / STUBS:
NOT STARTED:
TESTS EXECUTED (exact commands/results):
TESTS NOT EXECUTED (and why):
KNOWN ISSUES:
SAVE COMPATIBILITY:
HOW TO RUN:
NEXT CONCRETE TASK:
```

No claims of working gameplay, `.exe`, screenshots, ZIP or benchmarks without real artifacts/evidence.
