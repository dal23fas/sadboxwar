// Authoritative domain states (World Sim §1, §3). Visual actors are projections of these.
// All numeric values are fictional game balance values.
#pragma once
#include "EFWorldCore/Json.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>

namespace ef {

using EntityId = std::string;
using Tick = int64_t;

constexpr int kSaveSchemaVersion = 1;

// ---------- geometry ----------
struct Vec3 {
    double x = 0, y = 0, z = 0;
    Json toJson() const;
    static Vec3 fromJson(const Json& j);
};
double distance(const Vec3& a, const Vec3& b);

struct Pose {
    Vec3 position;
    Vec3 rotation_euler_deg;
    Json toJson() const;
    static Pose fromJson(const Json& j);
};

// ---------- static content definitions (data-driven, not saved) ----------
struct ItemDef {
    std::string id, label, category;
    int base_price_credits = 0;
    int mass_game_units = 1;
};

struct PoiDef {
    std::string id, name, kind;   // kind: town | port | airfield | clinic | depot | hamlet
    EntityId region_id;
    Pose pose;
};

struct NpcDef {
    std::string id, name, role_tag;
    EntityId poi_id, faction_id;
};

struct SeatDef {
    std::string seat_id;   // driver | pilot | captain | passenger ...
    bool is_control_seat = false;
};

enum class VehicleClass { Ground, Armored, Rotorcraft, FixedWing, Naval, Unknown };
const char* vehicleClassName(VehicleClass c);
VehicleClass vehicleClassFromName(const std::string& s);

struct VehicleDefinition {
    std::string definition_id, display_name, control_profile;
    VehicleClass vehicle_class = VehicleClass::Unknown;
    std::vector<SeatDef> seats;
    int cargo_capacity_game_units = 0;
    bool hasSeat(const std::string& seatId) const;
    const SeatDef* seat(const std::string& seatId) const;
};

enum class ObjectiveType { Interact, TransportCargo, Unknown };
const char* objectiveTypeName(ObjectiveType t);
ObjectiveType objectiveTypeFromName(const std::string& s);

struct ObjectiveDef {
    std::string id;
    ObjectiveType type = ObjectiveType::Unknown;
    EntityId target_id;            // Interact: npc id
    std::string item_id;           // TransportCargo
    int quantity = 0;              // TransportCargo
    EntityId destination_node_id;  // TransportCargo
};

struct MissionRewards {
    int credits = 0;
    int local_trust_delta = 0;
    int faction_reputation_delta = 0;
};

struct MissionTemplate {
    std::string id, title, type;   // type: civilian_relief | transport | story ...
    EntityId issuer_faction_id;
    EntityId region_id;
    std::vector<VehicleClass> eligible_vehicle_classes;
    std::vector<ObjectiveDef> objectives;
    MissionRewards rewards;
    int min_local_trust = -100;
    std::vector<std::string> required_completed_missions;
    // Which NPC offers it: the first Interact objective's target.
    EntityId offeringNpc() const;
};

struct InfrastructureDef {
    std::string id, kind;  // kind: port | airfield | road
    EntityId poi_id, region_id;
};

struct WorldContent;  // defined after the mutable states (it carries their initial values)

// ---------- mutable, saved state ----------
struct VehicleState {
    EntityId entity_id, definition_id, region_id, home_poi_id;
    Pose pose;
    double condition_normalized = 1.0;
    double fuel_normalized = 1.0;
    std::map<std::string, int> cargo;                 // item -> qty
    std::map<std::string, EntityId> occupied_seats;   // seat_id -> occupant entity
    int cargoUnits(const WorldContent& c) const;
};

struct InfrastructureState {
    EntityId id;
    std::string status = "operational";   // operational | restricted | closed
};

struct RegionState {
    EntityId id;
    std::string name;
    EntityId control_faction_id;
    double stability_normalized = 0.5;
    int population_index = 0;
    int local_trust = 0;                  // community reputation, -100..100
    EntityId spawn_poi_id;
    std::vector<EntityId> economy_node_ids, poi_ids;
};

struct FactionState {
    EntityId id;
    std::string name;
    int reputation = 0;                   // -100..100
};

const char* reputationBand(int value);  // Hostile / Distrusted / Neutral / Trusted / Allied

struct EconomyNodeState {
    EntityId id, region_id, poi_id;
    std::map<std::string, int> stocks, capacity, target_stock;
    std::map<std::string, int> production_per_step, consumption_per_step;  // per Tier-R step
    int stock(const std::string& item) const;
    int capacityOf(const std::string& item) const;
    double shortage(const std::string& item) const;       // World Sim §8
    int priceHint(const ItemDef& item, double shortage_weight = 1.5) const;
};

struct WeatherState {
    EntityId region_id;
    std::string condition = "clear";     // clear | rain | storm
    Tick last_change_tick = 0;
};

enum class MissionStatus { Locked, Available, Active, Completed, Failed };
const char* missionStatusName(MissionStatus s);
MissionStatus missionStatusFromName(const std::string& s);

struct MissionState {
    EntityId mission_id;
    MissionStatus status = MissionStatus::Available;
    int current_objective_index = 0;
    int delivered_quantity = 0;          // progress of current TransportCargo objective
    Tick accepted_tick = -1, completed_tick = -1;
    std::string completion_event_id;
};

struct NewsItem {
    std::string news_id, headline;
    Tick tick = 0;
    std::string source_event_id;
    EntityId region_id;
};

struct PlayerState {
    EntityId entity_id = "player_main";
    std::string display_name = "Operator";
    EntityId region_id;
    Pose pose;                            // valid when on foot
    int credits = 0;
    int carry_capacity_game_units = 12;
    std::map<std::string, int> inventory;
    std::set<std::string> qualifications;
    std::vector<std::string> career_history;
    int inventoryUnits(const WorldContent& c) const;
};

enum class PossessionMode { OnFoot, Seated };
struct PossessionState {
    PossessionMode mode = PossessionMode::OnFoot;
    EntityId vehicle_id, seat_id;
    std::string role = "infantry";        // derived label for HUD/profile
};

struct WorldTime {
    Tick tick = 0;
    int ticks_per_hour = 60;              // 1 tick = 1 game minute
    int start_hour = 8;
    int day() const { return static_cast<int>(tick / (ticks_per_hour * 24)); }
    int hour() const { return static_cast<int>((start_hour + tick / ticks_per_hour) % 24); }
    int minute() const { return static_cast<int>(tick % ticks_per_hour); }
};

struct CampaignState {
    std::string act = "ACT_I_FRACTURED_PEACE";
    std::map<std::string, std::string> flags;
};

struct SimulationConfig {
    int tier_r_interval_ticks = 10;       // regional/economy step
    int tier_g_interval_ticks = 60;       // global/weather/politics step
    Tick max_catchup_ticks_per_call = 7 * 24 * 60;
    double interact_radius = 30.0;
    double board_radius = 40.0;
    double delivery_radius = 40.0;
    double airborne_exit_height = 5.0;    // placeholder: rotorcraft above this z cannot be exited
};

// ---------- content package: definitions + initial state (data-driven, loaded from JSON) ----------
struct WorldContent {
    std::string content_id;   // e.g. "asterion_sample_v1"
    uint64_t simulation_seed = 0;
    EntityId starting_region_id;
    EntityId starting_poi_id;
    int starting_credits = 0;
    std::map<std::string, ItemDef> items;
    std::map<std::string, PoiDef> pois;
    std::map<std::string, NpcDef> npcs;
    std::map<std::string, VehicleDefinition> vehicle_definitions;
    std::map<std::string, MissionTemplate> mission_templates;
    std::map<std::string, InfrastructureDef> infrastructure;
    // Dialogue lines keyed by npc id -> condition tag -> text (localization keys in later phases)
    std::map<std::string, std::map<std::string, std::string>> dialogue;
    // Initial authoritative state for StartNewGame
    std::vector<RegionState> initial_regions;
    std::vector<FactionState> initial_factions;
    std::vector<EconomyNodeState> initial_economy_nodes;
    std::vector<VehicleState> initial_vehicles;
};

} // namespace ef
