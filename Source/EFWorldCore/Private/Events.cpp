#include "EFWorldCore/Events.h"

namespace ef {

namespace {
struct NamePair { EventType t; const char* n; };
const NamePair kNames[] = {
    {EventType::RoleChanged, "RoleChanged"},
    {EventType::VehicleEntered, "VehicleEntered"},
    {EventType::VehicleExited, "VehicleExited"},
    {EventType::VehicleConditionChanged, "VehicleConditionChanged"},
    {EventType::MissionAccepted, "MissionAccepted"},
    {EventType::MissionCompleted, "MissionCompleted"},
    {EventType::MissionFailed, "MissionFailed"},
    {EventType::CommunityReputationChanged, "CommunityReputationChanged"},
    {EventType::FactionReputationChanged, "FactionReputationChanged"},
    {EventType::RegionStabilityChanged, "RegionStabilityChanged"},
    {EventType::RegionControlChanged, "RegionControlChanged"},
    {EventType::InfrastructureStatusChanged, "InfrastructureStatusChanged"},
    {EventType::ResourceStockChanged, "ResourceStockChanged"},
    {EventType::CivilianReliefDelivered, "CivilianReliefDelivered"},
    {EventType::WeatherChanged, "WeatherChanged"},
    {EventType::CampaignFlagChanged, "CampaignFlagChanged"},
    {EventType::WorldNewsPublished, "WorldNewsPublished"},
    {EventType::MissionObjectiveProgressed, "MissionObjectiveProgressed"},
    {EventType::MissionObjectiveCompleted, "MissionObjectiveCompleted"},
    {EventType::NpcInteracted, "NpcInteracted"},
    {EventType::EntityPoseChanged, "EntityPoseChanged"},
    {EventType::VehicleCargoChanged, "VehicleCargoChanged"},
    {EventType::PlayerInventoryChanged, "PlayerInventoryChanged"},
    {EventType::PlayerCreditsChanged, "PlayerCreditsChanged"},
    {EventType::VehicleFuelChanged, "VehicleFuelChanged"},
    {EventType::MissionGenerated, "MissionGenerated"},
};
} // namespace

const char* eventTypeName(EventType t) {
    for (const auto& p : kNames) if (p.t == t) return p.n;
    return "Unknown";
}

EventType eventTypeFromName(const std::string& name) {
    for (const auto& p : kNames) if (name == p.n) return p.t;
    return EventType::Unknown;
}

Json EventEnvelope::toJson() const {
    Json j = Json::object();
    j.set("event_id", event_id);
    j.set("event_type", eventTypeName(event_type));
    j.set("schema_version", schema_version);
    j.set("occurred_at_world_tick", static_cast<int64_t>(occurred_at_world_tick));
    j.set("source_entity_id", source_entity_id);
    j.set("region_id", region_id);
    j.set("payload", payload);
    return j;
}

bool EventEnvelope::fromJson(const Json& j, EventEnvelope& out, std::string* error) {
    if (!j.isObject()) { if (error) *error = "event is not an object"; return false; }
    out.event_id = j.get("event_id").asString();
    out.event_type = eventTypeFromName(j.get("event_type").asString());
    out.schema_version = static_cast<int>(j.get("schema_version").asInt(kEventSchemaVersion));
    out.occurred_at_world_tick = j.get("occurred_at_world_tick").asInt(-1);
    out.source_entity_id = j.get("source_entity_id").asString();
    out.region_id = j.get("region_id").asString();
    out.payload = j.get("payload").isObject() ? j.get("payload") : Json::object();
    Status s = out.validate();
    if (!s) { if (error) *error = s.message; return false; }
    return true;
}

Status EventEnvelope::validate() const {
    if (event_id.empty()) return Status::Fail("event_invalid", "empty event_id");
    if (event_type == EventType::Unknown) return Status::Fail("event_invalid", "unknown event_type");
    if (schema_version < 1 || schema_version > kEventSchemaVersion) return Status::Fail("event_invalid", "unsupported schema_version");
    if (occurred_at_world_tick < 0) return Status::Fail("event_invalid", "negative world tick");
    if (source_entity_id.empty()) return Status::Fail("event_invalid", "empty source_entity_id");
    if (!payload.isObject()) return Status::Fail("event_invalid", "payload must be object");
    return Status::Ok();
}

bool EventJournal::append(const EventEnvelope& e) {
    if (applied_.count(e.event_id)) return false;
    applied_.insert(e.event_id);
    events_.push_back(e);
    return true;
}

std::vector<EventEnvelope> EventJournal::lastN(size_t n) const {
    if (n >= events_.size()) return events_;
    return std::vector<EventEnvelope>(events_.end() - static_cast<long>(n), events_.end());
}

Json EventJournal::toJson() const {
    Json j = Json::object();
    j.set("sequence", static_cast<uint64_t>(sequence_));
    Json arr = Json::array();
    for (const auto& e : events_) arr.push(e.toJson());
    j.set("events", std::move(arr));
    return j;
}

bool EventJournal::fromJson(const Json& j, std::string* error) {
    clear();
    sequence_ = static_cast<uint64_t>(j.get("sequence").asInt(0));
    for (const auto& ej : j.get("events").asArray()) {
        EventEnvelope e;
        if (!EventEnvelope::fromJson(ej, e, error)) return false;
        append(e);
    }
    return true;
}

} // namespace ef
