// WorldState: lifecycle, command handlers (validation) and reducers (applyEvent).
#include "EFWorldCore/WorldState.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace ef {

// ============================================================ lifecycle
Status WorldState::initFromContent(const WorldContent& content) {
    if (content.content_id.empty()) return Status::Fail("content_invalid", "missing content_id");
    if (!content.pois.count(content.starting_poi_id)) return Status::Fail("content_invalid", "starting_poi_id not found");
    content_ = content;
    seed_ = content.simulation_seed;
    time_ = WorldTime{};
    player_ = PlayerState{};
    player_.credits = content.starting_credits;
    player_.region_id = content.starting_region_id;
    player_.pose = content.pois.at(content.starting_poi_id).pose;
    possession_ = PossessionState{};
    campaign_ = CampaignState{};
    regions_.clear(); factions_.clear(); economy_.clear(); vehicles_.clear(); generated_.clear();
    missions_.clear(); infra_.clear(); weather_.clear(); news_.clear(); journal_.clear();
    for (const auto& r : content.initial_regions) regions_[r.id] = r;
    for (const auto& f : content.initial_factions) factions_[f.id] = f;
    for (const auto& n : content.initial_economy_nodes) economy_[n.id] = n;
    for (const auto& v : content.initial_vehicles) {
        if (!content.vehicle_definitions.count(v.definition_id))
            return Status::Fail("content_invalid", "vehicle definition missing: " + v.definition_id);
        vehicles_[v.entity_id] = v;
    }
    for (const auto& kv : content.infrastructure) { InfrastructureState s; s.id = kv.first; infra_[kv.first] = s; }
    for (const auto& kv : regions_) { WeatherState w; w.region_id = kv.first; weather_[kv.first] = w; }
    for (const auto& kv : content.mission_templates) { MissionState m; m.mission_id = kv.first; missions_[kv.first] = m; }
    if (!regions_.count(player_.region_id)) return Status::Fail("content_invalid", "starting_region_id not found");
    initialized_ = true;
    return Status::Ok();
}

// ============================================================ lookups
const RegionState* WorldState::region(const EntityId& id) const { auto it = regions_.find(id); return it == regions_.end() ? nullptr : &it->second; }
const FactionState* WorldState::faction(const EntityId& id) const { auto it = factions_.find(id); return it == factions_.end() ? nullptr : &it->second; }
const EconomyNodeState* WorldState::economyNode(const EntityId& id) const { auto it = economy_.find(id); return it == economy_.end() ? nullptr : &it->second; }
const VehicleState* WorldState::vehicle(const EntityId& id) const { auto it = vehicles_.find(id); return it == vehicles_.end() ? nullptr : &it->second; }
const MissionState* WorldState::mission(const EntityId& id) const { auto it = missions_.find(id); return it == missions_.end() ? nullptr : &it->second; }
const InfrastructureState* WorldState::infrastructureState(const EntityId& id) const { auto it = infra_.find(id); return it == infra_.end() ? nullptr : &it->second; }
const WeatherState* WorldState::regionWeather(const EntityId& id) const { auto it = weather_.find(id); return it == weather_.end() ? nullptr : &it->second; }
const PoiDef* WorldState::poi(const EntityId& id) const { auto it = content_.pois.find(id); return it == content_.pois.end() ? nullptr : &it->second; }
const MissionTemplate* WorldState::missionDef(const EntityId& id) const {
    auto it = content_.mission_templates.find(id); if (it != content_.mission_templates.end()) return &it->second;
    auto g = generated_.find(id); return g == generated_.end() ? nullptr : &g->second;
}
const PoiDef* WorldState::nearestPoiOfKinds(const Vec3& p, const std::vector<std::string>& kinds, double radius) const {
    const PoiDef* best = nullptr; double bestD = radius;
    for (const auto& kv : content_.pois) {
        bool ok = false; for (const auto& k : kinds) if (kv.second.kind == k) ok = true;
        if (!ok) continue;
        double d = distance(p, kv.second.pose.position);
        if (d <= bestD) { bestD = d; best = &kv.second; }
    }
    return best;
}
const NpcDef* WorldState::npc(const EntityId& id) const { auto it = content_.npcs.find(id); return it == content_.npcs.end() ? nullptr : &it->second; }
const VehicleDefinition* WorldState::vehicleDef(const EntityId& vehicleId) const {
    const VehicleState* v = vehicle(vehicleId); if (!v) return nullptr;
    auto it = content_.vehicle_definitions.find(v->definition_id);
    return it == content_.vehicle_definitions.end() ? nullptr : &it->second;
}

Pose WorldState::playerEffectivePose() const {
    if (possession_.mode == PossessionMode::Seated) { const VehicleState* v = vehicle(possession_.vehicle_id); if (v) return v->pose; }
    return player_.pose;
}

const InfrastructureState* WorldState::infrastructureNear(const Vec3& p, const std::string& kind, double radius) const {
    const InfrastructureState* best = nullptr; double bestD = radius;
    for (const auto& kv : content_.infrastructure) {
        if (kv.second.kind != kind) continue;
        const PoiDef* pd = poi(kv.second.poi_id); if (!pd) continue;
        double d = distance(p, pd->pose.position);
        if (d <= bestD) { bestD = d; best = infrastructureState(kv.first); }
    }
    return best;
}

std::string WorldState::requiredInfrastructureKind(VehicleClass c) const {
    switch (c) { case VehicleClass::Naval: case VehicleClass::Submarine: return "port"; case VehicleClass::Rotorcraft: case VehicleClass::FixedWing: return "airfield"; default: return ""; }
}

// A transport mode is blocked in a region when every infrastructure of the required kind there
// (port for naval, airfield for aviation) is restricted/closed (WS-05 / WORLD-02). Regions without
// such infrastructure impose no rule. Ground vehicles are never restricted by this rule.
Status WorldState::checkTransportMode(VehicleClass c, const EntityId& regionId) const {
    std::string kind = requiredInfrastructureKind(c);
    if (kind.empty()) return Status::Ok();
    bool any = false; std::string blockedStatus, blockedId;
    for (const auto& kv : content_.infrastructure) {
        if (kv.second.kind != kind || kv.second.region_id != regionId) continue;
        const InfrastructureState* s = infrastructureState(kv.first); if (!s) continue;
        any = true;
        if (s->status == "operational") return Status::Ok();
        blockedStatus = s->status; blockedId = kv.first;
    }
    if (!any) return Status::Ok();
    return Status::Fail(kind + "_" + blockedStatus, kind + " " + blockedId + " is " + blockedStatus + " (weather gameplay rule)");
}

bool WorldState::missionPrerequisitesMet(const MissionTemplate& def, std::string* why) const {
    const RegionState* r = region(def.region_id.empty() ? player_.region_id : def.region_id);
    if (r && r->local_trust < def.min_local_trust) { if (why) *why = "local_trust_too_low"; return false; }
    for (const auto& req : def.required_completed_missions) {
        const MissionState* m = mission(req);
        if (!m || m->status != MissionStatus::Completed) { if (why) *why = "requires_" + req; return false; }
    }
    return true;
}

std::map<std::string, int>& WorldState::currentCarrier(int* capacityUnits) {
    if (possession_.mode == PossessionMode::Seated) {
        auto it = vehicles_.find(possession_.vehicle_id);
        if (it != vehicles_.end()) {
            const VehicleDefinition* d = vehicleDef(it->first);
            if (capacityUnits) *capacityUnits = d ? d->cargo_capacity_game_units : 0;
            return it->second.cargo;
        }
    }
    if (capacityUnits) *capacityUnits = player_.carry_capacity_game_units;
    return player_.inventory;
}

static std::string roleForSeat(const VehicleDefinition& def, const std::string& seatId) {
    const SeatDef* s = def.seat(seatId);
    if (!s || !s->is_control_seat) return "passenger";
    switch (def.vehicle_class) {
        case VehicleClass::Ground: return "driver";
        case VehicleClass::Armored: return "armored_crew";
        case VehicleClass::Rotorcraft: case VehicleClass::FixedWing: return "pilot";
        case VehicleClass::Naval: case VehicleClass::Submarine: return "captain";
        default: return "operator";
    }
}

// ============================================================ command dispatch
CommandResult WorldState::apply(const Command& cmd) {
    CommandResult result;
    if (!initialized_) { result.status = Status::Fail("not_initialized", "call initFromContent first"); return result; }
    std::vector<EventEnvelope> primary;
    Status st = std::visit([&](const auto& c) { return handle(c, primary); }, cmd);
    if (!st) { result.status = st; return result; }  // nothing mutated
    for (const auto& e : primary) commitWithReactions(e, result.committed_event_ids);
    result.status = Status::Ok();
    return result;
}

Status WorldState::ingestEvent(const EventEnvelope& e, bool* applied) {
    Status v = e.validate();
    if (!v) { if (applied) *applied = false; return v; }
    if (journal_.contains(e.event_id)) { if (applied) *applied = false; return Status::Ok(); }
    std::vector<std::string> ids;
    commitWithReactions(e, ids);
    if (applied) *applied = true;
    return Status::Ok();
}

EventEnvelope WorldState::makeEvent(EventType t, const EntityId& source, const EntityId& regionId, Json payload) {
    EventEnvelope e;
    char buf[32]; std::snprintf(buf, sizeof buf, "evt_%07llu", static_cast<unsigned long long>(journal_.nextSequence()));
    e.event_id = buf;
    e.event_type = t;
    e.occurred_at_world_tick = time_.tick;
    e.source_entity_id = source;
    e.region_id = regionId;
    e.payload = std::move(payload);
    return e;
}

EventEnvelope WorldState::derive(EventType t, const EntityId& source, const EntityId& regionId, Json payload) const {
    EventEnvelope e;
    e.event_type = t;
    e.occurred_at_world_tick = time_.tick;
    e.source_entity_id = source;
    e.region_id = regionId;
    e.payload = std::move(payload);
    return e;   // event_id assigned by commitWithReactions
}

void WorldState::commitWithReactions(const EventEnvelope& e, std::vector<std::string>& committedIds) {
    if (!commit(e)) return;                   // duplicate: no mutation, no reactions
    committedIds.push_back(e.event_id);
    std::vector<EventEnvelope> derived = react(e);
    for (size_t i = 0; i < derived.size(); ++i) {
        derived[i].event_id = e.event_id + "/" + std::to_string(i + 1);
        if (derived[i].event_type == EventType::WorldNewsPublished) derived[i].payload.set("news_id", "news_" + derived[i].event_id);
        commitWithReactions(derived[i], committedIds);
    }
}

bool WorldState::commit(const EventEnvelope& e) {
    if (journal_.contains(e.event_id)) return false;
    applyEvent(e);
    journal_.append(e);
    return true;
}

// ============================================================ handlers (validate, build events)
Status WorldState::handle(const CmdInteractNpc& c, std::vector<EventEnvelope>& out) {
    const NpcDef* n = npc(c.npc_id);
    if (!n) return Status::Fail("npc_not_found", c.npc_id);
    if (possession_.mode != PossessionMode::OnFoot) return Status::Fail("must_be_on_foot", "exit the vehicle to talk");
    const PoiDef* p = poi(n->poi_id);
    if (!p) return Status::Fail("npc_location_unknown", n->poi_id);
    if (distance(player_.pose.position, p->pose.position) > config_.interact_radius) return Status::Fail("too_far", "approach the NPC");
    Json pl = Json::object(); pl.set("npc_id", c.npc_id);
    out.push_back(makeEvent(EventType::NpcInteracted, player_.entity_id, p->region_id, pl));
    return Status::Ok();
}

Status WorldState::handle(const CmdAcceptMission& c, std::vector<EventEnvelope>& out) {
    const MissionTemplate* defp = missionDef(c.mission_id);
    if (!defp) return Status::Fail("mission_not_found", c.mission_id);
    const MissionTemplate& def = *defp;
    const MissionState* st = mission(c.mission_id);
    if (!st || st->status != MissionStatus::Available) return Status::Fail("mission_not_available", missionStatusName(st ? st->status : MissionStatus::Locked));
    std::string why; if (!missionPrerequisitesMet(def, &why)) return Status::Fail("prerequisites_not_met", why);
    const NpcDef* n = npc(def.offeringNpc());
    if (n && possession_.mode != PossessionMode::OnFoot) return Status::Fail("must_be_on_foot");
    if (n) {
        const PoiDef* p = poi(n->poi_id);
        if (!p || distance(player_.pose.position, p->pose.position) > config_.interact_radius) return Status::Fail("too_far", "talk to " + n->name);
    }
    Json pl = Json::object(); pl.set("mission_id", c.mission_id); pl.set("issuer_faction_id", def.issuer_faction_id);
    out.push_back(makeEvent(EventType::MissionAccepted, player_.entity_id, def.region_id.empty() ? player_.region_id : def.region_id, pl));
    return Status::Ok();
}

Status WorldState::handle(const CmdEnterVehicle& c, std::vector<EventEnvelope>& out) {
    if (possession_.mode != PossessionMode::OnFoot) return Status::Fail("already_seated", possession_.vehicle_id);
    const VehicleState* v = vehicle(c.vehicle_id);
    if (!v) return Status::Fail("vehicle_not_found", c.vehicle_id);
    const VehicleDefinition* d = vehicleDef(c.vehicle_id);
    if (!d) return Status::Fail("vehicle_definition_missing", v->definition_id);
    if (!d->hasSeat(c.seat_id)) return Status::Fail("seat_not_found", c.seat_id);
    if (v->occupied_seats.count(c.seat_id)) return Status::Fail("seat_occupied", c.seat_id);
    if (distance(player_.pose.position, v->pose.position) > config_.board_radius) return Status::Fail("too_far", "approach the vehicle");
    if (v->condition_normalized <= 0.05) return Status::Fail("vehicle_unusable", "needs repair");
    Status mode = checkTransportMode(d->vehicle_class, v->region_id);
    if (!mode) return mode;
    Json pl = Json::object();
    pl.set("vehicle_id", c.vehicle_id); pl.set("seat_id", c.seat_id); pl.set("occupant_entity_id", player_.entity_id);
    out.push_back(makeEvent(EventType::VehicleEntered, player_.entity_id, v->region_id, pl));
    Json rl = Json::object();
    rl.set("previous_role", possession_.role); rl.set("new_role", roleForSeat(*d, c.seat_id));
    rl.set("mode", "seated"); rl.set("vehicle_id", c.vehicle_id); rl.set("seat_id", c.seat_id);
    out.push_back(makeEvent(EventType::RoleChanged, player_.entity_id, v->region_id, rl));
    return Status::Ok();
}

Status WorldState::handle(const CmdExitVehicle&, std::vector<EventEnvelope>& out) {
    if (possession_.mode != PossessionMode::Seated) return Status::Fail("not_seated");
    const VehicleState* v = vehicle(possession_.vehicle_id);
    if (!v) return Status::Fail("vehicle_not_found", possession_.vehicle_id);
    const VehicleDefinition* d = vehicleDef(possession_.vehicle_id);
    if (d && (d->vehicle_class == VehicleClass::Rotorcraft || d->vehicle_class == VehicleClass::FixedWing) && v->pose.position.z > config_.airborne_exit_height)
        return Status::Fail("exit_unsafe_airborne", "land first");
    Pose exitPose = v->pose; exitPose.position.x += 3.0; exitPose.position.z = 0.0;
    Json pl = Json::object();
    pl.set("vehicle_id", possession_.vehicle_id); pl.set("seat_id", possession_.seat_id);
    pl.set("occupant_entity_id", player_.entity_id); pl.set("exit_pose", exitPose.toJson());
    out.push_back(makeEvent(EventType::VehicleExited, player_.entity_id, v->region_id, pl));
    Json rl = Json::object();
    rl.set("previous_role", possession_.role); rl.set("new_role", "infantry"); rl.set("mode", "on_foot");
    out.push_back(makeEvent(EventType::RoleChanged, player_.entity_id, v->region_id, rl));
    return Status::Ok();
}

Status WorldState::handle(const CmdSwitchSeat& c, std::vector<EventEnvelope>& out) {
    if (possession_.mode != PossessionMode::Seated) return Status::Fail("not_seated");
    const VehicleState* v = vehicle(possession_.vehicle_id);
    const VehicleDefinition* d = vehicleDef(possession_.vehicle_id);
    if (!v || !d) return Status::Fail("vehicle_not_found");
    if (!d->hasSeat(c.seat_id)) return Status::Fail("seat_not_found", c.seat_id);
    if (c.seat_id == possession_.seat_id) return Status::Fail("already_in_seat");
    if (v->occupied_seats.count(c.seat_id)) return Status::Fail("seat_occupied", c.seat_id);
    Json pl = Json::object();
    pl.set("vehicle_id", possession_.vehicle_id); pl.set("seat_id", c.seat_id);
    pl.set("previous_seat_id", possession_.seat_id); pl.set("occupant_entity_id", player_.entity_id);
    out.push_back(makeEvent(EventType::VehicleEntered, player_.entity_id, v->region_id, pl));
    Json rl = Json::object();
    rl.set("previous_role", possession_.role); rl.set("new_role", roleForSeat(*d, c.seat_id));
    rl.set("mode", "seated"); rl.set("vehicle_id", possession_.vehicle_id); rl.set("seat_id", c.seat_id);
    out.push_back(makeEvent(EventType::RoleChanged, player_.entity_id, v->region_id, rl));
    return Status::Ok();
}

Status WorldState::handle(const CmdReportPose& c, std::vector<EventEnvelope>& out) {
    EntityId regionId;
    if (c.entity_id == player_.entity_id) {
        if (possession_.mode != PossessionMode::OnFoot) return Status::Fail("player_pose_owned_by_vehicle", "report the vehicle pose instead");
        regionId = player_.region_id;
    } else {
        const VehicleState* v = vehicle(c.entity_id);
        if (!v) return Status::Fail("entity_not_found", c.entity_id);
        regionId = v->region_id;
    }
    Json pl = Json::object(); pl.set("entity_id", c.entity_id); pl.set("pose", c.pose.toJson());
    out.push_back(makeEvent(EventType::EntityPoseChanged, c.entity_id, regionId, pl));
    if (c.entity_id != player_.entity_id) {
        const VehicleState* v = vehicle(c.entity_id); const VehicleDefinition* d = vehicleDef(c.entity_id);
        if (v && d && d->range_km > 0.0) {
            double meters = distance(v->pose.position, c.pose.position);
            if (meters > 0.01 && meters < 5000.0) {   // teleports / load corrections do not burn fuel
                double nf = std::max(0.0, v->fuel_normalized - meters / (d->range_km * 1000.0));
                if (nf != v->fuel_normalized) {
                    Json fl = Json::object(); fl.set("vehicle_id", c.entity_id); fl.set("previous", v->fuel_normalized); fl.set("new", nf); fl.set("reason_tag", "travel");
                    out.push_back(makeEvent(EventType::VehicleFuelChanged, "system_vehicles", regionId, fl));
                }
            }
        }
    }
    return Status::Ok();
}

Status WorldState::handle(const CmdRefuelVehicle& c, std::vector<EventEnvelope>& out) {
    const VehicleState* v = vehicle(c.vehicle_id); const VehicleDefinition* d = vehicleDef(c.vehicle_id);
    if (!v || !d) return Status::Fail("vehicle_not_found", c.vehicle_id);
    if (distance(playerEffectivePose().position, v->pose.position) > config_.board_radius) return Status::Fail("too_far", "stand next to the vehicle");
    if (!nearestPoiOfKinds(v->pose.position, {"depot", "airfield", "port", "town"}, config_.delivery_radius)) return Status::Fail("no_service_here", "refuel at a depot, airfield, port or town");
    if (v->fuel_normalized >= 0.999) return Status::Fail("tank_full");
    int cost = static_cast<int>(std::lround((1.0 - v->fuel_normalized) * d->refuel_cost_credits));
    if (player_.credits < cost) return Status::Fail("insufficient_credits", std::to_string(cost) + " needed");
    Json fl = Json::object(); fl.set("vehicle_id", c.vehicle_id); fl.set("previous", v->fuel_normalized); fl.set("new", 1.0); fl.set("reason_tag", "refuel");
    out.push_back(makeEvent(EventType::VehicleFuelChanged, player_.entity_id, v->region_id, fl));
    if (cost > 0) { Json cr = Json::object(); cr.set("previous", player_.credits); cr.set("new", player_.credits - cost); cr.set("delta", -cost); cr.set("reason_tag", "refuel"); out.push_back(makeEvent(EventType::PlayerCreditsChanged, player_.entity_id, v->region_id, cr)); }
    return Status::Ok();
}

Status WorldState::handle(const CmdRepairVehicle& c, std::vector<EventEnvelope>& out) {
    const VehicleState* v = vehicle(c.vehicle_id); const VehicleDefinition* d = vehicleDef(c.vehicle_id);
    if (!v || !d) return Status::Fail("vehicle_not_found", c.vehicle_id);
    if (distance(playerEffectivePose().position, v->pose.position) > config_.board_radius) return Status::Fail("too_far", "stand next to the vehicle");
    const PoiDef* at = nearestPoiOfKinds(v->pose.position, {"depot", "airfield", "port"}, config_.delivery_radius);
    if (!at) return Status::Fail("no_service_here", "repair at a depot, airfield or port");
    if (v->condition_normalized >= 0.999) return Status::Fail("no_damage");
    double deficit = 1.0 - v->condition_normalized;
    int cost = static_cast<int>(std::lround(deficit * d->repair_cost_credits));
    int parts = static_cast<int>(std::ceil(deficit * 4.0));
    if (player_.credits < cost) return Status::Fail("insufficient_credits", std::to_string(cost) + " needed");
    // spare parts come from the nearest economy node that has them (any node within 400 m of the service POI)
    const EconomyNodeState* src = nullptr;
    for (const auto& kv : economy_) { const PoiDef* np = poi(kv.second.poi_id); if (np && distance(np->pose.position, at->pose.position) <= 400.0 && kv.second.stock("spare_parts") >= parts) { src = &kv.second; break; } }
    if (!src) return Status::Fail("no_spare_parts", std::to_string(parts) + " spare parts needed nearby");
    Json st = Json::object(); st.set("node_id", src->id); st.set("item_id", "spare_parts"); st.set("previous_quantity", src->stock("spare_parts")); st.set("new_quantity", src->stock("spare_parts") - parts); st.set("delta", -parts); st.set("reason_tag", "repair");
    out.push_back(makeEvent(EventType::ResourceStockChanged, player_.entity_id, v->region_id, st));
    Json cd = Json::object(); cd.set("vehicle_id", c.vehicle_id); cd.set("previous", v->condition_normalized); cd.set("new", 1.0); cd.set("reason_tag", "repair");
    out.push_back(makeEvent(EventType::VehicleConditionChanged, player_.entity_id, v->region_id, cd));
    if (cost > 0) { Json cr = Json::object(); cr.set("previous", player_.credits); cr.set("new", player_.credits - cost); cr.set("delta", -cost); cr.set("reason_tag", "repair"); out.push_back(makeEvent(EventType::PlayerCreditsChanged, player_.entity_id, v->region_id, cr)); }
    return Status::Ok();
}

Status WorldState::handle(const CmdLoadCargo& c, std::vector<EventEnvelope>& out) {
    if (c.quantity <= 0) return Status::Fail("invalid_quantity");
    const EconomyNodeState* node = economyNode(c.node_id);
    if (!node) return Status::Fail("node_not_found", c.node_id);
    auto item = content_.items.find(c.item_id);
    if (item == content_.items.end()) return Status::Fail("item_not_found", c.item_id);
    if (node->stock(c.item_id) < c.quantity) return Status::Fail("insufficient_stock", std::to_string(node->stock(c.item_id)) + " available");
    const PoiDef* p = poi(node->poi_id);
    if (!p || distance(playerEffectivePose().position, p->pose.position) > config_.delivery_radius) return Status::Fail("too_far", "move to the facility");
    int capacity = 0; std::map<std::string, int>& carrier = currentCarrier(&capacity);
    int used = possession_.mode == PossessionMode::Seated ? vehicles_[possession_.vehicle_id].cargoUnits(content_) : player_.inventoryUnits(content_);
    if (used + c.quantity * item->second.mass_game_units > capacity) return Status::Fail("carrier_capacity_exceeded", std::to_string(capacity - used) + " units free");
    int prevCarrier = carrier.count(c.item_id) ? carrier.at(c.item_id) : 0;
    Json stock = Json::object();
    stock.set("node_id", c.node_id); stock.set("item_id", c.item_id);
    stock.set("previous_quantity", node->stock(c.item_id)); stock.set("new_quantity", node->stock(c.item_id) - c.quantity);
    stock.set("delta", -c.quantity); stock.set("reason_tag", "player_load");
    out.push_back(makeEvent(EventType::ResourceStockChanged, player_.entity_id, node->region_id, stock));
    Json carry = Json::object();
    carry.set("item_id", c.item_id); carry.set("previous_quantity", prevCarrier); carry.set("new_quantity", prevCarrier + c.quantity); carry.set("delta", c.quantity);
    if (possession_.mode == PossessionMode::Seated) { carry.set("vehicle_id", possession_.vehicle_id); out.push_back(makeEvent(EventType::VehicleCargoChanged, player_.entity_id, node->region_id, carry)); }
    else out.push_back(makeEvent(EventType::PlayerInventoryChanged, player_.entity_id, node->region_id, carry));
    return Status::Ok();
}

Status WorldState::handle(const CmdDeliverCargo& c, std::vector<EventEnvelope>& out) {
    if (c.quantity <= 0) return Status::Fail("invalid_quantity");
    const EconomyNodeState* node = economyNode(c.node_id);
    if (!node) return Status::Fail("node_not_found", c.node_id);
    if (!content_.items.count(c.item_id)) return Status::Fail("item_not_found", c.item_id);
    const PoiDef* p = poi(node->poi_id);
    if (!p || distance(playerEffectivePose().position, p->pose.position) > config_.delivery_radius) return Status::Fail("too_far", "move to the facility");
    int capacity = 0; std::map<std::string, int>& carrier = currentCarrier(&capacity);
    int have = carrier.count(c.item_id) ? carrier.at(c.item_id) : 0;
    if (have < c.quantity) return Status::Fail("insufficient_cargo", std::to_string(have) + " carried");
    int cap = node->capacityOf(c.item_id);
    if (cap > 0 && node->stock(c.item_id) + c.quantity > cap) return Status::Fail("node_capacity_exceeded", std::to_string(cap - node->stock(c.item_id)) + " accepted");
    Json carry = Json::object();
    carry.set("item_id", c.item_id); carry.set("previous_quantity", have); carry.set("new_quantity", have - c.quantity); carry.set("delta", -c.quantity);
    if (possession_.mode == PossessionMode::Seated) { carry.set("vehicle_id", possession_.vehicle_id); out.push_back(makeEvent(EventType::VehicleCargoChanged, player_.entity_id, node->region_id, carry)); }
    else out.push_back(makeEvent(EventType::PlayerInventoryChanged, player_.entity_id, node->region_id, carry));
    Json stock = Json::object();
    stock.set("node_id", c.node_id); stock.set("item_id", c.item_id);
    stock.set("previous_quantity", node->stock(c.item_id)); stock.set("new_quantity", node->stock(c.item_id) + c.quantity);
    stock.set("delta", c.quantity); stock.set("reason_tag", "player_delivery");
    if (possession_.mode == PossessionMode::Seated) { const VehicleDefinition* d = vehicleDef(possession_.vehicle_id); stock.set("vehicle_class", d ? vehicleClassName(d->vehicle_class) : "unknown"); }
    else stock.set("vehicle_class", "on_foot");
    out.push_back(makeEvent(EventType::ResourceStockChanged, player_.entity_id, node->region_id, stock));
    return Status::Ok();
}

Status WorldState::handle(const CmdSetVehicleCondition& c, std::vector<EventEnvelope>& out) {
    const VehicleState* v = vehicle(c.vehicle_id);
    if (!v) return Status::Fail("vehicle_not_found", c.vehicle_id);
    double nv = std::clamp(c.condition_normalized, 0.0, 1.0);
    if (nv == v->condition_normalized) return Status::Ok();
    Json pl = Json::object();
    pl.set("vehicle_id", c.vehicle_id); pl.set("previous", v->condition_normalized); pl.set("new", nv); pl.set("reason_tag", c.reason_tag);
    out.push_back(makeEvent(EventType::VehicleConditionChanged, "system_vehicles", v->region_id, pl));
    return Status::Ok();
}

Status WorldState::handle(const CmdSetWeather& c, std::vector<EventEnvelope>& out) {
    const WeatherState* w = regionWeather(c.region_id);
    if (!w) return Status::Fail("region_not_found", c.region_id);
    if (c.condition != "clear" && c.condition != "rain" && c.condition != "storm") return Status::Fail("invalid_weather", c.condition);
    if (c.condition == w->condition) return Status::Ok();
    Json pl = Json::object(); pl.set("region_id", c.region_id); pl.set("previous", w->condition); pl.set("new", c.condition);
    out.push_back(makeEvent(EventType::WeatherChanged, "system_weather", c.region_id, pl));
    return Status::Ok();
}

Status WorldState::handle(const CmdSetCampaignFlag& c, std::vector<EventEnvelope>& out) {
    if (c.flag.empty()) return Status::Fail("invalid_flag");
    auto it = campaign_.flags.find(c.flag);
    std::string prev = it == campaign_.flags.end() ? "" : it->second;
    if (prev == c.value) return Status::Ok();
    Json pl = Json::object(); pl.set("flag", c.flag); pl.set("previous", prev); pl.set("new", c.value);
    out.push_back(makeEvent(EventType::CampaignFlagChanged, "system_campaign", player_.region_id, pl));
    return Status::Ok();
}

// ============================================================ reducers
// Pure state mutation. Must never fail and must never emit; replaying the journal through
// applyEvent reproduces the snapshot.
void WorldState::applyEvent(const EventEnvelope& e) {
    const Json& p = e.payload;
    switch (e.event_type) {
        case EventType::RoleChanged:
            possession_.role = p.get("new_role").asString();
            player_.career_history.push_back(possession_.role);
            if (player_.career_history.size() > 64) player_.career_history.erase(player_.career_history.begin());
            break;
        case EventType::VehicleEntered: {
            auto it = vehicles_.find(p.get("vehicle_id").asString());
            if (it == vehicles_.end()) break;
            if (p.has("previous_seat_id")) it->second.occupied_seats.erase(p.get("previous_seat_id").asString());
            it->second.occupied_seats[p.get("seat_id").asString()] = p.get("occupant_entity_id").asString();
            if (p.get("occupant_entity_id").asString() == player_.entity_id) {
                possession_.mode = PossessionMode::Seated;
                possession_.vehicle_id = it->first;
                possession_.seat_id = p.get("seat_id").asString();
            }
            break;
        }
        case EventType::VehicleExited: {
            auto it = vehicles_.find(p.get("vehicle_id").asString());
            if (it != vehicles_.end()) it->second.occupied_seats.erase(p.get("seat_id").asString());
            if (p.get("occupant_entity_id").asString() == player_.entity_id) {
                possession_.mode = PossessionMode::OnFoot;
                possession_.vehicle_id.clear(); possession_.seat_id.clear();
                player_.pose = Pose::fromJson(p.get("exit_pose"));
                if (it != vehicles_.end()) player_.region_id = it->second.region_id;
            }
            break;
        }
        case EventType::VehicleConditionChanged: {
            auto it = vehicles_.find(p.get("vehicle_id").asString());
            if (it != vehicles_.end()) it->second.condition_normalized = std::clamp(p.get("new").asNumber(), 0.0, 1.0);
            break;
        }
        case EventType::MissionAccepted: {
            auto it = missions_.find(p.get("mission_id").asString());
            if (it != missions_.end() && it->second.status == MissionStatus::Available) {
                it->second.status = MissionStatus::Active; it->second.accepted_tick = e.occurred_at_world_tick;
                it->second.current_objective_index = 0; it->second.delivered_quantity = 0;
            }
            break;
        }
        case EventType::MissionObjectiveProgressed: {
            auto it = missions_.find(p.get("mission_id").asString());
            if (it != missions_.end()) it->second.delivered_quantity = static_cast<int>(p.get("delivered_total").asInt());
            break;
        }
        case EventType::MissionObjectiveCompleted: {
            auto it = missions_.find(p.get("mission_id").asString());
            if (it != missions_.end() && it->second.status == MissionStatus::Active) {
                it->second.current_objective_index = static_cast<int>(p.get("objective_index").asInt()) + 1;
                it->second.delivered_quantity = 0;
            }
            break;
        }
        case EventType::MissionCompleted: {
            auto it = missions_.find(p.get("mission_id").asString());
            if (it != missions_.end() && it->second.status != MissionStatus::Completed) {
                it->second.status = MissionStatus::Completed; it->second.completed_tick = e.occurred_at_world_tick;
                it->second.completion_event_id = e.event_id;
            }
            break;
        }
        case EventType::MissionFailed: {
            auto it = missions_.find(p.get("mission_id").asString());
            if (it != missions_.end() && (it->second.status == MissionStatus::Active || it->second.status == MissionStatus::Available)) {
                const MissionTemplate* d = missionDef(it->first);
                bool retry = d && d->retryable && !d->generated && it->second.status == MissionStatus::Active;
                it->second.status = retry ? MissionStatus::Available : MissionStatus::Failed;
                it->second.current_objective_index = 0; it->second.delivered_quantity = 0; it->second.accepted_tick = -1;
            }
            break;
        }
        case EventType::MissionGenerated: {
            MissionTemplate t; std::string err;
            if (MissionTemplate::fromJson(p.get("template"), t, &err) && !content_.mission_templates.count(t.id)) {
                t.generated = true; generated_[t.id] = t;
                if (!missions_.count(t.id)) { MissionState m; m.mission_id = t.id; missions_[t.id] = m; }
            }
            break;
        }
        case EventType::VehicleFuelChanged: {
            auto it = vehicles_.find(p.get("vehicle_id").asString());
            if (it != vehicles_.end()) it->second.fuel_normalized = std::clamp(p.get("new").asNumber(), 0.0, 1.0);
            break;
        }
        case EventType::CommunityReputationChanged: {
            auto it = regions_.find(p.get("region_id").asString());
            if (it != regions_.end()) it->second.local_trust = std::clamp(static_cast<int>(p.get("new").asInt()), -100, 100);
            break;
        }
        case EventType::FactionReputationChanged: {
            auto it = factions_.find(p.get("faction_id").asString());
            if (it != factions_.end()) it->second.reputation = std::clamp(static_cast<int>(p.get("new").asInt()), -100, 100);
            break;
        }
        case EventType::RegionStabilityChanged: {
            auto it = regions_.find(p.get("region_id").asString());
            if (it != regions_.end()) it->second.stability_normalized = std::clamp(p.get("new").asNumber(), 0.0, 1.0);
            break;
        }
        case EventType::RegionControlChanged: {
            auto it = regions_.find(p.get("region_id").asString());
            if (it != regions_.end()) it->second.control_faction_id = p.get("new_faction_id").asString();
            break;
        }
        case EventType::InfrastructureStatusChanged: {
            auto it = infra_.find(p.get("infrastructure_id").asString());
            if (it != infra_.end()) it->second.status = p.get("new_status").asString();
            break;
        }
        case EventType::ResourceStockChanged: {
            auto it = economy_.find(p.get("node_id").asString());
            if (it != economy_.end()) {
                const std::string item = p.get("item_id").asString();
                int nv = static_cast<int>(p.get("new_quantity").asInt());
                int cap = it->second.capacityOf(item);
                it->second.stocks[item] = std::max(0, cap > 0 ? std::min(nv, cap) : nv);
            }
            break;
        }
        case EventType::CivilianReliefDelivered: break;   // informational (mission/economy already applied)
        case EventType::WeatherChanged: {
            auto it = weather_.find(p.get("region_id").asString());
            if (it != weather_.end()) { it->second.condition = p.get("new").asString(); it->second.last_change_tick = e.occurred_at_world_tick; }
            break;
        }
        case EventType::CampaignFlagChanged:
            if (p.get("flag").asString() == "act") campaign_.act = p.get("new").asString();
            else campaign_.flags[p.get("flag").asString()] = p.get("new").asString();
            break;
        case EventType::WorldNewsPublished: {
            NewsItem n; n.news_id = p.get("news_id").asString(); n.headline = p.get("headline").asString();
            n.tick = e.occurred_at_world_tick; n.source_event_id = p.get("source_event_id").asString(); n.region_id = e.region_id;
            news_.push_back(n);
            break;
        }
        case EventType::NpcInteracted: break;
        case EventType::EntityPoseChanged: {
            const std::string id = p.get("entity_id").asString();
            Pose pose = Pose::fromJson(p.get("pose"));
            // region follows the nearest POI (regions are not streaming chunks, TDD §9)
            EntityId nearestRegion; double best = 1e300;
            for (const auto& kv : content_.pois) { double d = distance(pose.position, kv.second.pose.position); if (d < best) { best = d; nearestRegion = kv.second.region_id; } }
            if (id == player_.entity_id) { player_.pose = pose; if (!nearestRegion.empty()) player_.region_id = nearestRegion; }
            else { auto it = vehicles_.find(id); if (it != vehicles_.end()) { it->second.pose = pose; if (!nearestRegion.empty()) it->second.region_id = nearestRegion; } }
            break;
        }
        case EventType::VehicleCargoChanged: {
            auto it = vehicles_.find(p.get("vehicle_id").asString());
            if (it != vehicles_.end()) {
                int nv = static_cast<int>(p.get("new_quantity").asInt());
                if (nv <= 0) it->second.cargo.erase(p.get("item_id").asString()); else it->second.cargo[p.get("item_id").asString()] = nv;
            }
            break;
        }
        case EventType::PlayerInventoryChanged: {
            int nv = static_cast<int>(p.get("new_quantity").asInt());
            if (nv <= 0) player_.inventory.erase(p.get("item_id").asString()); else player_.inventory[p.get("item_id").asString()] = nv;
            break;
        }
        case EventType::PlayerCreditsChanged:
            player_.credits = static_cast<int>(p.get("new").asInt());
            break;
        case EventType::Unknown: break;
    }
}

} // namespace ef
