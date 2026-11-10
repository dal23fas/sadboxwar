// The single authoritative world state. "One game, one truth" (master prompt §37).
//
// Mutation path:  Command --validate--> [EventEnvelope...] --commit--> reducer(applyEvent) + journal
// Derived effects (mission progress, rewards, news, infrastructure) are themselves events,
// produced by react() after each committed event, so the journal is a complete audit trail and
// replaying it over fresh content reproduces the same state (tested).
#pragma once
#include "EFWorldCore/Domain.h"
#include "EFWorldCore/Events.h"
#include "EFWorldCore/Commands.h"
#include <map>
#include <string>
#include <vector>
#include <functional>

namespace ef {

struct MissionBoardEntry {
    const MissionTemplate* def = nullptr;
    MissionState state;
    std::vector<VehicleClass> allowed_vehicle_classes;   // eligible minus weather/infrastructure restrictions
    std::string blocked_reason;                           // non-empty if not acceptable right now
};

struct SimulationReport {
    Tick ticks_advanced = 0;
    Tick ticks_remaining = 0;     // >0 if capped by max_catchup_ticks_per_call
    int tier_r_steps = 0, tier_g_steps = 0;
    std::vector<std::string> committed_event_ids;
};

class WorldState {
public:
    WorldState() = default;

    // ----- lifecycle -----
    // StartNewGame: builds initial state from content (CORE-01).
    Status initFromContent(const WorldContent& content);
    const WorldContent& content() const { return content_; }

    // ----- commands -----
    CommandResult apply(const Command& cmd);
    // Ingest an externally produced / replayed event. Duplicate IDs are ignored (WS-01).
    // Returns ok with applied=false when duplicate.
    Status ingestEvent(const EventEnvelope& e, bool* applied = nullptr);

    // ----- simulation -----
    SimulationReport advanceTicks(Tick ticks);
    SimulationConfig& config() { return config_; }
    const SimulationConfig& config() const { return config_; }

    // ----- read-only projections (UI must only read these) -----
    const WorldTime& time() const { return time_; }
    const PlayerState& player() const { return player_; }
    const PossessionState& possession() const { return possession_; }
    const CampaignState& campaign() const { return campaign_; }
    const std::map<EntityId, RegionState>& regions() const { return regions_; }
    const std::map<EntityId, FactionState>& factions() const { return factions_; }
    const std::map<EntityId, EconomyNodeState>& economyNodes() const { return economy_; }
    const std::map<EntityId, VehicleState>& vehicles() const { return vehicles_; }
    const std::map<EntityId, MissionState>& missions() const { return missions_; }
    const MissionTemplate* missionDef(const EntityId& id) const;   // content template or runtime-generated
    const std::map<EntityId, MissionTemplate>& generatedMissions() const { return generated_; }
    const std::map<EntityId, InfrastructureState>& infrastructure() const { return infra_; }
    const std::map<EntityId, WeatherState>& weather() const { return weather_; }
    const std::vector<NewsItem>& news() const { return news_; }
    const EventJournal& journal() const { return journal_; }
    uint64_t seed() const { return seed_; }

    const RegionState* region(const EntityId& id) const;
    const FactionState* faction(const EntityId& id) const;
    const EconomyNodeState* economyNode(const EntityId& id) const;
    const VehicleState* vehicle(const EntityId& id) const;
    const MissionState* mission(const EntityId& id) const;
    const InfrastructureState* infrastructureState(const EntityId& id) const;
    const WeatherState* regionWeather(const EntityId& regionId) const;

    Pose playerEffectivePose() const;                       // own pose, or possessed vehicle pose
    std::vector<MissionBoardEntry> missionBoard() const;
    std::string npcDialogue(const EntityId& npcId) const;   // chosen by authoritative state
    std::string debugSummary() const;                        // World Sim §11 observability

    // ----- persistence (used by SaveService) -----
    Json toSnapshot() const;
    Status restoreSnapshot(const Json& snapshot, std::vector<std::string>* warnings = nullptr);

private:
    // command handlers (validate + build primary events)
    Status handle(const CmdInteractNpc&, std::vector<EventEnvelope>&);
    Status handle(const CmdAcceptMission&, std::vector<EventEnvelope>&);
    Status handle(const CmdEnterVehicle&, std::vector<EventEnvelope>&);
    Status handle(const CmdExitVehicle&, std::vector<EventEnvelope>&);
    Status handle(const CmdSwitchSeat&, std::vector<EventEnvelope>&);
    Status handle(const CmdReportPose&, std::vector<EventEnvelope>&);
    Status handle(const CmdLoadCargo&, std::vector<EventEnvelope>&);
    Status handle(const CmdDeliverCargo&, std::vector<EventEnvelope>&);
    Status handle(const CmdSetVehicleCondition&, std::vector<EventEnvelope>&);
    Status handle(const CmdSetWeather&, std::vector<EventEnvelope>&);
    Status handle(const CmdSetCampaignFlag&, std::vector<EventEnvelope>&);
    Status handle(const CmdRefuelVehicle&, std::vector<EventEnvelope>&);
    Status handle(const CmdRepairVehicle&, std::vector<EventEnvelope>&);

    // event pipeline
    // Primary events get sequential IDs (evt_0000042). Derived events get deterministic IDs
    // "<cause_id>/<n>", so replaying a journal re-derives the same IDs and dedup makes replay idempotent.
    EventEnvelope makeEvent(EventType t, const EntityId& source, const EntityId& region, Json payload);
    EventEnvelope derive(EventType t, const EntityId& source, const EntityId& region, Json payload) const;
    void commitWithReactions(const EventEnvelope& e, std::vector<std::string>& committedIds);
    bool commit(const EventEnvelope& e);                 // apply + journal; false if duplicate
    void applyEvent(const EventEnvelope& e);             // pure reducer, never fails
    std::vector<EventEnvelope> react(const EventEnvelope& e);  // derived events

    // reactions helpers
    void reactMissionProgress(const EventEnvelope& e, std::vector<EventEnvelope>& out);
    void completeMission(const MissionTemplate& def, const MissionState& st, const EventEnvelope& cause, std::vector<EventEnvelope>& out);
    void reactWeatherToInfrastructure(const EventEnvelope& e, std::vector<EventEnvelope>& out);
    void reactNews(const EventEnvelope& e, std::vector<EventEnvelope>& out);
    EventEnvelope newsEvent(const std::string& headline, const EventEnvelope& source);

    // simulation steps
    void stepTierR(std::vector<std::string>& committed);
    void stepTierG(std::vector<std::string>& committed);
    void stepMissionTimers(std::vector<EventEnvelope>& out);
    void stepMissionGenerator(std::vector<EventEnvelope>& out);

    // helpers
    const PoiDef* poi(const EntityId& id) const;
    const PoiDef* nearestPoiOfKinds(const Vec3& p, const std::vector<std::string>& kinds, double radius) const;
    const NpcDef* npc(const EntityId& id) const;
    const VehicleDefinition* vehicleDef(const EntityId& vehicleId) const;
    const InfrastructureState* infrastructureNear(const Vec3& p, const std::string& kind, double radius) const;
    std::string requiredInfrastructureKind(VehicleClass c) const;
    Status checkTransportMode(VehicleClass c, const EntityId& regionId) const;
    bool missionPrerequisitesMet(const MissionTemplate& def, std::string* why) const;
    std::map<std::string, int>& currentCarrier(int* capacityUnits);

    WorldContent content_;
    SimulationConfig config_;
    uint64_t seed_ = 0;
    WorldTime time_;
    PlayerState player_;
    PossessionState possession_;
    CampaignState campaign_;
    std::map<EntityId, RegionState> regions_;
    std::map<EntityId, FactionState> factions_;
    std::map<EntityId, EconomyNodeState> economy_;
    std::map<EntityId, VehicleState> vehicles_;
    std::map<EntityId, MissionState> missions_;
    std::map<EntityId, MissionTemplate> generated_;
    std::map<EntityId, InfrastructureState> infra_;
    std::map<EntityId, WeatherState> weather_;
    std::vector<NewsItem> news_;
    EventJournal journal_;
    bool initialized_ = false;
};

} // namespace ef
