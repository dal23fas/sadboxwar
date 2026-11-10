# Data Contracts v1 — Events, Commands, Snapshot

Υλοποίηση: `Source/EFWorldCore/`. Όλα τα παρακάτω είναι **εκτελέσιμος, δοκιμασμένος κώδικας**, όχι σχέδιο.

## 1. Αρχή μετάλλαξης (TDD §7, World Sim §6)

```
Command ──validate──► [primary events] ──commit──► applyEvent() + journal.append()
                                                        │
                                                        └── react() ──► [derived events] ──commit──► ...
```

- **Command** απορρίπτεται με `Status{code,message}` χωρίς καμία μετάλλαξη (test `ws10_rejected_commands_do_not_mutate`).
- **Reducer** (`applyEvent`) είναι η *μοναδική* διαδρομή μετάλλαξης. Ποτέ δεν αποτυγχάνει, ποτέ δεν εκπέμπει.
- **Derived events** (πρόοδος αποστολής, ανταμοιβές, υποδομές, news) παράγονται από `react()` με **ντετερμινιστικά IDs** `"<cause_id>/<n>"`. Έτσι το replay του journal πάνω σε φρέσκο content ξαναπαράγει ίδια IDs και η dedup του journal καθιστά το replay idempotent (test `ws04_..._replay_matches`).
- **Primary IDs:** `evt_0000042` από monotonic sequence που αποθηκεύεται στο save.

## 2. Event envelope

| Πεδίο | Τύπος | Κανόνας |
|---|---|---|
| `event_id` | string | μη κενό, μοναδικό ανά journal |
| `event_type` | string enum | γνωστό όνομα (βλ. §3) |
| `schema_version` | int | 1 |
| `occurred_at_world_tick` | int ≥ 0 | world tick (1 tick = 1 λεπτό παιχνιδιού) |
| `source_entity_id` | string | `player_main`, `system_economy`, `system_weather`, `system_missions`, `system_news`, `system_simulation_director`, `system_vehicles`, `system_campaign` |
| `region_id` | string | περιοχή αναφοράς |
| `payload` | object | ανά τύπο (βλ. §3) |

Malformed envelope → `event_invalid`, καμία μετάλλαξη (test `envelope_validation_rejects_malformed`).

## 3. Event types και payloads

Από master prompt §38 (17) + 7 προσθήκες που απαίτησε το vertical slice (σημειωμένες με ★).

| Type | Owner (παράγει) | Payload |
|---|---|---|
| `RoleChanged` | Possession | `previous_role,new_role,mode,vehicle_id?,seat_id?` |
| `VehicleEntered` | Possession | `vehicle_id,seat_id,occupant_entity_id,previous_seat_id?` |
| `VehicleExited` | Possession | `vehicle_id,seat_id,occupant_entity_id,exit_pose` |
| `VehicleConditionChanged` | Vehicles | `vehicle_id,previous,new,reason_tag` |
| `MissionAccepted` | Missions | `mission_id,issuer_faction_id` |
| `MissionObjectiveProgressed` ★ | Missions | `mission_id,objective_id,delivered_delta,delivered_total,required` |
| `MissionObjectiveCompleted` ★ | Missions | `mission_id,objective_id,objective_index,cause_event_id` |
| `MissionCompleted` | Missions | `mission_id,title,issuer_faction_id,credits,local_trust_delta,faction_reputation_delta,cause_event_id` |
| `MissionFailed` | Missions | `mission_id,reason` (reducer έτοιμος, δεν εκπέμπεται ακόμη) |
| `CommunityReputationChanged` | Factions | `region_id,previous,new,delta,reason_mission_id` |
| `FactionReputationChanged` | Factions | `faction_id,previous,new,delta,reason_mission_id` |
| `RegionStabilityChanged` | SimulationDirector | `region_id,previous,new,mean_shortage,reason_tag` |
| `RegionControlChanged` | Factions | `region_id,new_faction_id` (reducer έτοιμος; Phase 6) |
| `InfrastructureStatusChanged` | Weather→Infra | `infrastructure_id,previous_status,new_status,reason_tag,cause_event_id` |
| `ResourceStockChanged` | Economy | `node_id,item_id,previous_quantity,new_quantity,delta,reason_tag,vehicle_class?` |
| `CivilianReliefDelivered` | Missions | `mission_id,item_id,quantity,node_id` |
| `WeatherChanged` | Weather | `region_id,previous,new,stream?` |
| `CampaignFlagChanged` | Campaign | `flag,previous,new` |
| `WorldNewsPublished` | News | `news_id,headline,source_event_id,source_event_type` |
| `NpcInteracted` ★ | Interaction | `npc_id` |
| `EntityPoseChanged` ★ | Presentation→Domain | `entity_id,pose{position[3],rotation[3]}` |
| `VehicleCargoChanged` ★ | Vehicles | `vehicle_id,item_id,previous_quantity,new_quantity,delta` |
| `PlayerInventoryChanged` ★ | Player | `item_id,previous_quantity,new_quantity,delta` |
| `PlayerCreditsChanged` ★ | Player | `previous,new,delta,reason_tag,mission_id?` |

## 4. Commands (Commands.h)

`InteractNpc`, `AcceptMission`, `EnterVehicle(vehicle,seat)`, `ExitVehicle`, `SwitchSeat`, `ReportPose(entity,pose)`, `LoadCargo(node,item,qty)`, `DeliverCargo(node,item,qty)`, `SetVehicleCondition`, `SetWeather` (system/debug), `SetCampaignFlag`.

Κανόνες επικύρωσης (επιλογή): απόσταση ≤ `interact_radius` (30) / `board_radius` (40) / `delivery_radius` (40); χωρητικότητα carrier (vehicle cargo ή inventory 12 units); απόθεμα/χωρητικότητα node; seat ελεύθερο; `exit_unsafe_airborne` για rotorcraft με z>5; `port_restricted`/`airfield_restricted` όταν η υποδομή της περιοχής δεν είναι operational.

**Ownership:** το presentation layer **δεν** γράφει θέσεις/αποθέματα — υποβάλλει `ReportPose` (χαμηλή συχνότητα) και commands. Ο engine-side physics είναι Tier L και δεν είναι source of truth για το domain.

## 5. Snapshot (SaveService)

```
{ "meta": {slot_name, content_id, schema_version, world_tick, saved_at_unix, build_label},
  "snapshot": { schema_version, content_id, simulation_seed, time, player, possession, campaign,
                regions[], factions[], economy_nodes[], vehicles[], missions[], infrastructure[],
                weather[], news[], journal{sequence, events[]} } }
```

- Atomic write: `.tmp` → verify re-parse → rotate προηγούμενο σε `.bak` → rename.
- Load: primary corrupt → rollback σε `.bak` με warning (test `corrupt_primary_rolls_back_to_backup`).
- Seated restore: αν vehicle/seat δεν υπάρχει → on-foot στο spawn της περιοχής + warning (test `save02`).
- Unsupported `schema_version` → `save_schema_unsupported`. Migration table: μόνο v1 προς το παρόν.

## 6. Simulation tiers (Simulation.cpp)

| Tier | Interval | Τι κάνει |
|---|---|---|
| L | engine fixed step | κίνηση/φυσική/animation (engine) |
| R | 10 ticks | `stock = clamp(stock + production − consumption, 0, capacity)` ανά node/item |
| G | 60 ticks | weather Markov chain με seeded stream `weather:<region>`; stability drift (−0.01 αν mean shortage > 0.6, αλλιώς +0.005 έως 0.95) |

Catch-up: `max_catchup_ticks_per_call` (default 1 εβδομάδα) — το `advanceTicks` επιστρέφει `ticks_remaining`.

## 7. Content package (Content/Data/asterion_island_v1.json)

`world, regions, factions, items, pois(pose σε μέτρα), npcs, vehicle_definitions, vehicles, infrastructure, economy_nodes(stocks/capacity/target/production/consumption), mission_templates(objectives: Interact|TransportCargo, rewards, prerequisites), dialogue(tags: default, offer, in_progress, debrief_ready, locked, completed, *_trusted)`.

Νέες παρατάξεις/αποστολές/οχήματα/NPC προστίθενται **μόνο με δεδομένα**, χωρίς αλλαγή core code (§19).
