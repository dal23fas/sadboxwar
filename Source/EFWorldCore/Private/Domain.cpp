#include "EFWorldCore/Domain.h"
#include <cmath>
#include <algorithm>

namespace ef {

Json Vec3::toJson() const { Json a = Json::array(); a.push(x); a.push(y); a.push(z); return a; }
Vec3 Vec3::fromJson(const Json& j) {
    Vec3 v; const auto& a = j.asArray();
    if (a.size() >= 3) { v.x = a[0].asNumber(); v.y = a[1].asNumber(); v.z = a[2].asNumber(); }
    return v;
}
double distance(const Vec3& a, const Vec3& b) {
    double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Json Pose::toJson() const { Json j = Json::object(); j.set("position", position.toJson()); j.set("rotation", rotation_euler_deg.toJson()); return j; }
Pose Pose::fromJson(const Json& j) { Pose p; p.position = Vec3::fromJson(j.get("position")); p.rotation_euler_deg = Vec3::fromJson(j.get("rotation")); return p; }

const char* vehicleClassName(VehicleClass c) {
    switch (c) {
        case VehicleClass::Ground: return "ground";
        case VehicleClass::Armored: return "armored";
        case VehicleClass::Rotorcraft: return "rotorcraft";
        case VehicleClass::FixedWing: return "fixed_wing";
        case VehicleClass::Naval: return "naval";
        default: return "unknown";
    }
}
VehicleClass vehicleClassFromName(const std::string& s) {
    if (s == "ground") return VehicleClass::Ground;
    if (s == "armored") return VehicleClass::Armored;
    if (s == "rotorcraft") return VehicleClass::Rotorcraft;
    if (s == "fixed_wing") return VehicleClass::FixedWing;
    if (s == "naval") return VehicleClass::Naval;
    return VehicleClass::Unknown;
}

bool VehicleDefinition::hasSeat(const std::string& seatId) const { return seat(seatId) != nullptr; }
const SeatDef* VehicleDefinition::seat(const std::string& seatId) const {
    for (const auto& s : seats) if (s.seat_id == seatId) return &s;
    return nullptr;
}

const char* objectiveTypeName(ObjectiveType t) {
    switch (t) { case ObjectiveType::Interact: return "Interact"; case ObjectiveType::TransportCargo: return "TransportCargo"; default: return "Unknown"; }
}
ObjectiveType objectiveTypeFromName(const std::string& s) {
    if (s == "Interact") return ObjectiveType::Interact;
    if (s == "TransportCargo") return ObjectiveType::TransportCargo;
    return ObjectiveType::Unknown;
}

EntityId MissionTemplate::offeringNpc() const {
    for (const auto& o : objectives) if (o.type == ObjectiveType::Interact) return o.target_id;
    return {};
}

int VehicleState::cargoUnits(const WorldContent& c) const {
    int total = 0;
    for (const auto& kv : cargo) { auto it = c.items.find(kv.first); total += kv.second * (it != c.items.end() ? it->second.mass_game_units : 1); }
    return total;
}
int PlayerState::inventoryUnits(const WorldContent& c) const {
    int total = 0;
    for (const auto& kv : inventory) { auto it = c.items.find(kv.first); total += kv.second * (it != c.items.end() ? it->second.mass_game_units : 1); }
    return total;
}

const char* reputationBand(int v) {
    if (v <= -60) return "Hostile";
    if (v <= -20) return "Distrusted";
    if (v < 20) return "Neutral";
    if (v < 60) return "Trusted";
    return "Allied";
}

int EconomyNodeState::stock(const std::string& item) const { auto it = stocks.find(item); return it == stocks.end() ? 0 : it->second; }
int EconomyNodeState::capacityOf(const std::string& item) const { auto it = capacity.find(item); return it == capacity.end() ? 0 : it->second; }
double EconomyNodeState::shortage(const std::string& item) const {
    auto it = target_stock.find(item);
    int target = it == target_stock.end() ? capacityOf(item) / 2 : it->second;
    if (target <= 0) return 0.0;
    double s = (static_cast<double>(target) - stock(item)) / static_cast<double>(std::max(target, 1));
    return std::clamp(s, 0.0, 1.0);
}
int EconomyNodeState::priceHint(const ItemDef& item, double shortage_weight) const {
    double p = item.base_price_credits * (1.0 + shortage_weight * shortage(item.id));
    return static_cast<int>(std::lround(p));
}

const char* missionStatusName(MissionStatus s) {
    switch (s) {
        case MissionStatus::Locked: return "Locked";
        case MissionStatus::Available: return "Available";
        case MissionStatus::Active: return "Active";
        case MissionStatus::Completed: return "Completed";
        case MissionStatus::Failed: return "Failed";
    }
    return "Available";
}
MissionStatus missionStatusFromName(const std::string& s) {
    if (s == "Locked") return MissionStatus::Locked;
    if (s == "Active") return MissionStatus::Active;
    if (s == "Completed") return MissionStatus::Completed;
    if (s == "Failed") return MissionStatus::Failed;
    return MissionStatus::Available;
}

} // namespace ef
