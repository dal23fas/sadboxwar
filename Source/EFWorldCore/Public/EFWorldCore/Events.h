// Event envelope + journal (master prompt §38, TDD §7, World Sim §5).
#pragma once
#include "EFWorldCore/Json.h"
#include "EFWorldCore/Result.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace ef {

using EntityId = std::string;
using Tick = int64_t;

constexpr int kEventSchemaVersion = 1;

// Contracts from §38 plus a few additional ones required by the vertical slice.
// Additions are documented in Docs/DATA_CONTRACTS.md.
enum class EventType {
    RoleChanged,
    VehicleEntered,
    VehicleExited,
    VehicleConditionChanged,
    MissionAccepted,
    MissionCompleted,
    MissionFailed,
    CommunityReputationChanged,
    FactionReputationChanged,
    RegionStabilityChanged,
    RegionControlChanged,
    InfrastructureStatusChanged,
    ResourceStockChanged,
    CivilianReliefDelivered,
    WeatherChanged,
    CampaignFlagChanged,
    WorldNewsPublished,
    // --- additions (vertical slice) ---
    MissionObjectiveProgressed,
    MissionObjectiveCompleted,
    NpcInteracted,
    EntityPoseChanged,
    VehicleCargoChanged,
    PlayerInventoryChanged,
    PlayerCreditsChanged,
    Unknown
};

const char* eventTypeName(EventType t);
EventType eventTypeFromName(const std::string& name);

struct EventEnvelope {
    std::string event_id;
    EventType event_type = EventType::Unknown;
    int schema_version = kEventSchemaVersion;
    Tick occurred_at_world_tick = 0;
    EntityId source_entity_id;
    EntityId region_id;
    Json payload = Json::object();

    Json toJson() const;
    static bool fromJson(const Json& j, EventEnvelope& out, std::string* error = nullptr);
    // Structural validation: type known, schema supported, non-negative tick, non-empty id/source.
    Status validate() const;
};

// Append-only journal with duplicate detection (WS-01 / EVT-01).
class EventJournal {
public:
    bool contains(const std::string& eventId) const { return applied_.count(eventId) != 0; }
    // Returns false if already applied.
    bool append(const EventEnvelope& e);
    const std::vector<EventEnvelope>& events() const { return events_; }
    size_t size() const { return events_.size(); }
    std::vector<EventEnvelope> lastN(size_t n) const;
    // Monotonic counter for generated IDs ("evt_0000042"); persisted in save.
    uint64_t nextSequence() { return ++sequence_; }
    uint64_t sequence() const { return sequence_; }
    void setSequence(uint64_t s) { sequence_ = s; }

    Json toJson() const;
    bool fromJson(const Json& j, std::string* error = nullptr);
    void clear() { events_.clear(); applied_.clear(); sequence_ = 0; }

private:
    std::vector<EventEnvelope> events_;
    std::unordered_set<std::string> applied_;
    uint64_t sequence_ = 0;
};

} // namespace ef
