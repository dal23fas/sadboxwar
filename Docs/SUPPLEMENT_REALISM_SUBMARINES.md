# Supplement v1.0 — Realism Modes, Vehicle Physics, Submarines
## Παραδοτέα Α–Ζ (συμπυκνωμένα, με δείκτες σε πραγματικό κώδικα)

Κατάσταση ανά παραδοτέο: **IMPLEMENTED** = κώδικας + test εδώ · **PROTOTYPE** = κώδικας Godot, μη εκτελεσμένος εδώ · **DESIGN** = μόνο κείμενο.

---

## Α. Συμπλήρωμα GDD — κατηγορίες οχημάτων/χαρακτήρα

| Κατηγορία (realism_category) | Vehicle classes (core) | Ρόλος control seat | Υποδομή αναχώρησης | Content δείγματα (2/κατηγορία) |
|---|---|---|---|---|
| infantry | — (player) | infantry | — | — |
| wheeled | ground | driver | καμία | utility_truck, light_4x4 |
| tracked | armored | armored_crew | καμία | tracked_transport, tracked_heavy |
| helicopter | rotorcraft | pilot | airfield | helicopter_light, helicopter_utility |
| fixed_wing | fixed_wing | pilot | airfield | prop_trainer, cargo_plane |
| surface_vessel | naval | captain | port | utility_patrol_boat, coastal_supply_ship |
| submarine | **submarine** (νέα κλάση) | captain | port | research_submarine |
| environment | κοινό layer | — | — | — |

Κανόνες κοινοί σε όλες: ένα `VehicleState` ανά όχημα (identity, seats, cargo, condition, pose) · possession μέσω `EnterVehicle/ExitVehicle/SwitchSeat` · οι αποστολές δηλώνουν `eligible_vehicle_classes` και ο κόσμος φιλτράρει με βάση υποδομή/καιρό, **όχι** με βάση realism preset.

Ο παίκτης (infantry): walk/sprint/jump, FPS/TPS ανεξάρτητα από preset (CORE-02), stamina ως ήπιο resource με `fatigue_rate`, `aim_assist` ως accessibility παράμετρος (combat Phase 7+).

## Β. Realism Modes — Technical Design

**Αρχή:** ένα base physics model ανά κλάση, assists ως στρώμα παραμέτρων. Το Arcade **δεν** απενεργοποιεί βαρύτητα/συγκρούσεις.

```
GameSettings (settings/realism.json — ΧΩΡΙΣΤΟ από world saves)
  └── RealismProfileManager  [IMPLEMENTED: EFWorldCore/RealismProfiles.*, 5 tests]
        global_preset ─► category preset override ─► custom sliders (0–100) ─► ResolvedProfile(category)
Presentation (Godot)
  └── vehicle.gd  [PROTOTYPE] base model από content.handling × ResolvedProfile(realism_category)
  └── player.gd   [PROTOTYPE] inertia_influence → accel smoothing, fatigue_rate → stamina drain
World (EFWorldCore)  — ΔΕΝ διαβάζει realism. Test: realism_settings_do_not_change_world_outcomes
```

Πώς κάθε slider επιδρά (vehicle.gd):

| Slider | wheeled/tracked | helicopter | surface/submarine |
|---|---|---|---|
| steering_assist | ταχύτητα απόκρισης τιμονιού (lerp 2.5→9) | response ώσης/yaw (0.8→4) | ρυθμός πηδαλίου (0.6→3) |
| auto_stabilization | μείωση υπερστροφής σε ταχύτητα | hover-hold (vertical→0), μείωση wind drift | μείωση heel, μείωση wave drift |
| inertia_influence | accel/brake ×(2.0→0.8), rolling resistance | ρυθμός αλλαγής ταχύτητας/vs | accel ×(2.0→0.6) |
| weather_sensitivity | — (Phase 2: grip) | wind drift 1.5 (rain) / 4.0 (storm) m/s | wave vector |
| depth_hold_assist | — | — | κέρδος/ρυθμός διόρθωσης βάθους |
| wave_influence / current_influence | — | — | κλίμακα wave vector |

Όλες οι παράμετροι clamp 0–100 στον core (`setOverride` απορρίπτει εκτός ορίων) → καμία NaN διαδρομή. Fixed timestep: Godot physics 60 Hz · world ticks ανεξάρτητα (1 Hz).

**Fallback σε αλλαγή preset εν κινήσει:** τα assists είναι παράμετροι ανά frame, όχι κατάσταση· αλλαγή preset δεν μηδενίζει `speed/vertical/steer` (T13 στο prototype). AI οχήματα (Phase 6) θα χρησιμοποιούν το ίδιο `vehicle.gd` με δικό τους input provider.

## Γ. Submarine System Design

**IMPLEMENTED (core):** `VehicleClass::Submarine`, role `captain`, εξάρτηση από `port`, eligible στο Harbor Link, content `research_submarine_fictional` (max_depth 120, dive_rate 1 m/s, seats captain/sensor_operator/passenger), POI `poi_reef_survey` (−40 m).  
**PROTOTYPE (Godot):** `_sail(..., submarine=true)` — depth_target με Space/Ctrl, PID-like διόρθωση κλιμακωμένη από `depth_hold_assist`, διαφορετική max ταχύτητα επιφάνειας/κατάδυσης, heel από πηδάλιο.  
**DESIGN (Phase 4):** control room interior (walkable), fictional sensing UI (abstract "contacts" από world state, όχι πραγματικά sonar μοντέλα), βυθομετρικός χάρτης παιχνιδιού (heightmap βυθού placeholder), ρεύματα ανά περιοχή από Weather tier, αποστολές: Reef Survey (Interact σε underwater POI), Deep Transport (TransportCargo σε παράκτιο node σε storm — μόνο υποβρύχιο εφικτό), Rescue Dive. Integrity ως `condition_normalized` με βάθος > max → φθορά (abstract).

## Δ. Settings UI schema (PROTOTYPE: godot/scripts/realism_settings.gd, F10)

```
SETTINGS › GAMEPLAY › REALISM & PHYSICS
  Global preset        [ARCADE|BALANCED|REALISTIC]        → realism_set_global
  Combat challenge     [EASY|NORMAL|HARD|CUSTOM]          → realism_set_combat   (ανεξάρτητο, §11)
  Per category (8)     [ARCADE|BALANCED|REALISTIC|CUSTOM] → realism_set_category
  Custom sliders for   <category>  slider ανά παράμετρο   → realism_set_override (drag end)
  [Reset category] [Reset ALL] [Save profile]             → realism_reset / realism_save
Tooltips: parameter_descriptions.el/en από το presets JSON (δίγλωσσα).
```
Η οθόνη διαβάζει μόνο `realism_get` projection· κάθε αλλαγή φαίνεται αμέσως (signal `realism_updated`). HUD assist levels & accessibility: πεδία υπάρχουν στο settings document, UI Phase 7.

## Ε. Configuration / Data

- `Content/Data/realism_presets_v1.json` — 8 κατηγορίες × 3 presets, περιγραφές el/en. Validation: ίδιο σύνολο παραμέτρων ανά preset, 0–100.
- `Content/Data/asterion_island_v1.json › vehicle_definitions[].handling` — base φυσικές παράμετροι, 11 ορισμοί (2 ανά κατηγορία + υποβρύχιο). Ο core τις μεταφέρει αδιάφανα (`VehicleDefinition.handling`).
- Settings αρχείο: `<saveDir>/settings/realism.json` (atomic write, schema_version 1). **Δεν** μπαίνει στο `.efsave`.

## ΣΤ. Αναθεωρημένο roadmap & vertical slice

| Phase | Περιεχόμενο (ενημερωμένο) | Κατάσταση |
|---|---|---|
| 0 Bootstrap | engine decision, core, contracts, tests, **RealismProfileManager + settings save/load** | DONE (core) |
| 1 Infantry | FPS/TPS, interact, HUD, save/load, **arcade/realistic feel στο player.gd** | PROTOTYPE (Godot, μη εκτελεσμένο εδώ) |
| 2 Vehicles | wheeled + **tracked** με 3 presets, cargo, garage | wheeled PROTOTYPE · tracked content only |
| 3 Aviation | helicopter + **fixed-wing** με presets, airfield rules | helicopter PROTOTYPE · fixed-wing content only |
| 4 Naval | surface vessel + **research submarine**, buoyancy, underwater controller, reef POI | PROTOTYPE (kinematic) |
| 5 Integration | ίδια αποστολή σε όλα τα role/mode combos, economy/reputation/news | DONE (core, headless) · Godot PROTOTYPE |
| 6 Living world | AI vehicles με ίδιο controller, faction director | PLANNED |
| 7 Campaign/roles | command UI, companions, interiors, rebinding UI, tutorials ανά mode | PLANNED |
| 8 Polish | assets, audio, localization, export (desktop + **Web**) | PLANNED |

**Vertical slice (ενημερωμένο):** Asterion Island + Tidewatch Reef (υποθαλάσσιο POI) + ερευνητικό υποβρύχιο στο Harbor West. Η διαδρομή του Roadmap §10 ισχύει· προστίθεται βήμα 7b: «Harbor Link με υποβρύχιο αντί για βάρκα δίνει τα ίδια events» (καλύπτεται ήδη από το core: ίδια `DeliverCargo` → ίδια `ResourceStockChanged`).

## Ζ. QA matrix — Arcade / Balanced / Realistic / Custom

| Test | Expected | Status | Evidence / γιατί όχι |
|---|---|---|---|
| T01 Arcade helicopter hover + camera | hover-hold, ασφαλής C | NOT_RUN | Godot prototype, απαιτεί editor |
| T02 Realistic helicopter inertia/weather | αισθητή καθυστέρηση, drift σε storm | NOT_RUN | ίδιο |
| T03 Arcade aircraft gamepad | — | NOT_STARTED | fixed-wing controller Phase 3 |
| T04 Realistic aircraft | — | NOT_STARTED | Phase 3 |
| T05 Tank weight/terrain | — | PARTIAL | tracked content + `_drive` παράμετροι· χωρίς terrain |
| T06 Surface ship waves/inertia | διαφορά preset στο wave vector | NOT_RUN | Godot prototype |
| T07 Submarine surface/dive, depth hold | προβλέψιμη διατήρηση βάθους | NOT_RUN | Godot prototype |
| T08 Submarine realistic lag/currents | — | PARTIAL | currents μόνο ως wind proxy |
| T09 FPS/TPS ίδιο objective σε Arcade/Realistic | ίδια events | **PASS_CORE** | `realism_settings_do_not_change_world_outcomes` |
| T10 Global + category override ταυτόχρονα | σωστή επίλυση | **PASS_CORE** | `t10_global_preset_and_category_override_work_together` |
| T11 Save/load κρατά world ΚΑΙ settings | settings χωριστό αρχείο, roundtrip | **PASS_CORE** (settings) + SAVE-01 (world) | `t16_..._settings_roundtrip` |
| T12 Αλλαγή ρόλου δεν μηδενίζει vehicle state | cargo/condition/pose διατηρούνται | **PASS_CORE** | ROLE-01, SAVE-02 |
| T13 Αλλαγή preset δεν καταστρέφει physics/mission | — | PARTIAL | mission: PASS_CORE (T09)· physics: Godot NOT_RUN |
| T14 AI off-screen | aggregate sim | **PASS_CORE** | WORLD-01 (economy/weather)· AI οχήματα Phase 6 |
| T15 Accessibility ανεξάρτητο από realism | settings πεδίο ανεξάρτητο | PASS_CORE (data) | `setAccessibility` · UI Phase 7 |
| T16 Reset επαναφέρει ακριβώς το preset | byte-identical resolved | **PASS_CORE** | `t16_reset_restores_exact_preset_and_settings_roundtrip` |

Engine version για τα PASS_CORE: κανένα engine — gcc 13.3 headless. Τα Godot NOT_RUN απαιτούν Godot 4.3+ στο μηχάνημα του χρήστη.
