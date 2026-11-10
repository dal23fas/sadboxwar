// Derived effects: mission progression, rewards, infrastructure reaction to weather, news.
// Everything here produces *events*; no direct state writes (World Sim §6 transaction chain).
#include "EFWorldCore/WorldState.h"
#include <algorithm>
#include <cstdio>

namespace ef {

std::vector<EventEnvelope> WorldState::react(const EventEnvelope& e) {
    std::vector<EventEnvelope> out;
    switch (e.event_type) {
        case EventType::NpcInteracted:
        case EventType::MissionAccepted:
        case EventType::ResourceStockChanged:
        case EventType::MissionObjectiveProgressed:
        case EventType::MissionObjectiveCompleted:
            reactMissionProgress(e, out);
            break;
        case EventType::MissionCompleted: {
            // Pay rewards exactly once: only the event that actually completed the mission (MISSION-01).
            const std::string mid = e.payload.get("mission_id").asString();
            const MissionState* st = mission(mid);
            const MissionTemplate* defp = missionDef(mid);
            if (!st || !defp || st->completion_event_id != e.event_id) break;
            const MissionTemplate& def = *defp;
            const EntityId regionId = def.region_id.empty() ? player_.region_id : def.region_id;
            if (def.rewards.credits != 0) {
                Json pl = Json::object(); pl.set("previous", player_.credits); pl.set("new", player_.credits + def.rewards.credits);
                pl.set("delta", def.rewards.credits); pl.set("reason_tag", "mission_reward"); pl.set("mission_id", mid);
                out.push_back(derive(EventType::PlayerCreditsChanged, "system_missions", regionId, pl));
            }
            if (const RegionState* r = region(regionId); r && def.rewards.local_trust_delta != 0) {
                int nv = std::clamp(r->local_trust + def.rewards.local_trust_delta, -100, 100);
                Json pl = Json::object(); pl.set("region_id", regionId); pl.set("previous", r->local_trust); pl.set("new", nv);
                pl.set("delta", nv - r->local_trust); pl.set("reason_mission_id", mid);
                out.push_back(derive(EventType::CommunityReputationChanged, "system_missions", regionId, pl));
            }
            if (const FactionState* f = faction(def.issuer_faction_id); f && def.rewards.faction_reputation_delta != 0) {
                int nv = std::clamp(f->reputation + def.rewards.faction_reputation_delta, -100, 100);
                Json pl = Json::object(); pl.set("faction_id", f->id); pl.set("previous", f->reputation); pl.set("new", nv);
                pl.set("delta", nv - f->reputation); pl.set("reason_mission_id", mid);
                out.push_back(derive(EventType::FactionReputationChanged, "system_missions", regionId, pl));
            }
            for (const auto& kv : def.campaign_flags_on_complete) {
                auto cur = campaign_.flags.find(kv.first);
                std::string prev = kv.first == "act" ? campaign_.act : (cur == campaign_.flags.end() ? "" : cur->second);
                if (prev == kv.second) continue;
                Json pl = Json::object(); pl.set("flag", kv.first); pl.set("previous", prev); pl.set("new", kv.second); pl.set("mission_id", mid);
                out.push_back(derive(EventType::CampaignFlagChanged, "system_campaign", regionId, pl));
            }
            reactNews(e, out);
            break;
        }
        case EventType::EntityPoseChanged:
            reactMissionProgress(e, out);
            break;
        case EventType::CampaignFlagChanged:
        case EventType::MissionGenerated:
        case EventType::MissionFailed:
            reactNews(e, out);
            break;
        case EventType::WeatherChanged:
            reactWeatherToInfrastructure(e, out);
            reactNews(e, out);
            break;
        case EventType::InfrastructureStatusChanged:
        case EventType::CivilianReliefDelivered:
        case EventType::RegionStabilityChanged:
        case EventType::RegionControlChanged:
            reactNews(e, out);
            break;
        default: break;
    }
    // Economy-driven shortage news (system source only; player deliveries are covered by relief news)
    if (e.event_type == EventType::ResourceStockChanged && e.source_entity_id == "system_economy") reactNews(e, out);
    return out;
}

void WorldState::reactMissionProgress(const EventEnvelope& e, std::vector<EventEnvelope>& out) {
    const Json& p = e.payload;
    for (const auto& kv : missions_) {
        const MissionState& st = kv.second;
        if (st.status != MissionStatus::Active) continue;
        const MissionTemplate* defp = missionDef(kv.first);
        if (!defp) continue;
        const MissionTemplate& def = *defp;
        if (st.current_objective_index < 0) continue;
        if (st.current_objective_index >= static_cast<int>(def.objectives.size())) {
            // All objectives done -> complete (only from the event that finished the last objective)
            if (e.event_type == EventType::MissionObjectiveCompleted && p.get("mission_id").asString() == kv.first)
                completeMission(def, st, e, out);
            continue;
        }
        const ObjectiveDef& obj = def.objectives[static_cast<size_t>(st.current_objective_index)];
        const EntityId regionId = def.region_id.empty() ? player_.region_id : def.region_id;
        auto objectiveDone = [&]() {
            Json pl = Json::object(); pl.set("mission_id", kv.first); pl.set("objective_id", obj.id); pl.set("objective_index", st.current_objective_index);
            pl.set("cause_event_id", e.event_id);
            out.push_back(derive(EventType::MissionObjectiveCompleted, "system_missions", regionId, pl));
        };
        switch (obj.type) {
            case ObjectiveType::Interact: {
                bool hit = false;
                if (e.event_type == EventType::NpcInteracted && p.get("npc_id").asString() == obj.target_id) hit = true;
                // Accepting a mission counts as the interaction with its offering NPC.
                if (e.event_type == EventType::MissionAccepted && p.get("mission_id").asString() == kv.first && obj.target_id == def.offeringNpc()) hit = true;
                if (hit) objectiveDone();
                break;
            }
            case ObjectiveType::TransportCargo: {
                if (e.event_type == EventType::ResourceStockChanged && e.source_entity_id == player_.entity_id
                    && p.get("node_id").asString() == obj.destination_node_id && p.get("item_id").asString() == obj.item_id
                    && p.get("delta").asInt() > 0) {
                    int delivered = st.delivered_quantity + static_cast<int>(p.get("delta").asInt());
                    Json pl = Json::object(); pl.set("mission_id", kv.first); pl.set("objective_id", obj.id);
                    pl.set("delivered_delta", static_cast<int64_t>(p.get("delta").asInt())); pl.set("delivered_total", delivered); pl.set("required", obj.quantity);
                    out.push_back(derive(EventType::MissionObjectiveProgressed, "system_missions", regionId, pl));
                }
                if (e.event_type == EventType::MissionObjectiveProgressed && p.get("mission_id").asString() == kv.first
                    && st.delivered_quantity >= obj.quantity) {
                    if (def.type == "civilian_relief") {
                        Json pl = Json::object(); pl.set("mission_id", kv.first); pl.set("item_id", obj.item_id);
                        pl.set("quantity", obj.quantity); pl.set("node_id", obj.destination_node_id);
                        out.push_back(derive(EventType::CivilianReliefDelivered, player_.entity_id, regionId, pl));
                    }
                    objectiveDone();
                }
                break;
            }
            case ObjectiveType::ReachLocation: {
                if (e.event_type != EventType::EntityPoseChanged) break;
                const std::string ent = p.get("entity_id").asString();
                const bool seated = possession_.mode == PossessionMode::Seated;
                // Only the entity the player is actually controlling counts.
                if (seated ? ent != possession_.vehicle_id : ent != player_.entity_id) break;
                if (!seated && !obj.on_foot_allowed) break;
                if (obj.required_vehicle_class != VehicleClass::Unknown) {
                    const VehicleDefinition* d = seated ? vehicleDef(possession_.vehicle_id) : nullptr;
                    if (!d || d->vehicle_class != obj.required_vehicle_class) break;
                }
                const PoiDef* target = poi(obj.target_id); if (!target) break;
                if (distance(Pose::fromJson(p.get("pose")).position, target->pose.position) <= obj.radius) objectiveDone();
                break;
            }
            default: break;
        }
    }
}

void WorldState::completeMission(const MissionTemplate& def, const MissionState& st, const EventEnvelope& cause, std::vector<EventEnvelope>& out) {
    if (st.status != MissionStatus::Active) return;
    Json pl = Json::object();
    pl.set("mission_id", def.id); pl.set("title", def.title); pl.set("issuer_faction_id", def.issuer_faction_id);
    pl.set("credits", def.rewards.credits); pl.set("local_trust_delta", def.rewards.local_trust_delta);
    pl.set("faction_reputation_delta", def.rewards.faction_reputation_delta); pl.set("cause_event_id", cause.event_id);
    out.push_back(derive(EventType::MissionCompleted, "system_missions", def.region_id.empty() ? player_.region_id : def.region_id, pl));
}

void WorldState::reactWeatherToInfrastructure(const EventEnvelope& e, std::vector<EventEnvelope>& out) {
    const std::string regionId = e.payload.get("region_id").asString();
    const std::string cond = e.payload.get("new").asString();
    for (const auto& kv : content_.infrastructure) {
        if (kv.second.region_id != regionId) continue;
        const InfrastructureState* s = infrastructureState(kv.first);
        if (!s) continue;
        std::string desired = "operational";
        if (cond == "storm" && (kv.second.kind == "port" || kv.second.kind == "airfield")) desired = "restricted";
        if (desired == s->status) continue;
        Json pl = Json::object();
        pl.set("infrastructure_id", kv.first); pl.set("previous_status", s->status); pl.set("new_status", desired);
        pl.set("reason_tag", "weather_gameplay_event"); pl.set("cause_event_id", e.event_id);
        out.push_back(derive(EventType::InfrastructureStatusChanged, "system_weather", regionId, pl));
    }
}

EventEnvelope WorldState::newsEvent(const std::string& headline, const EventEnvelope& source) {
    Json pl = Json::object();
    pl.set("news_id", "");   // final deterministic id assigned in commitWithReactions
    pl.set("headline", headline); pl.set("source_event_id", source.event_id);
    pl.set("source_event_type", eventTypeName(source.event_type));
    return derive(EventType::WorldNewsPublished, "system_news", source.region_id, pl);
}

static std::string poiNameOf(const WorldContent& c, const EntityId& poiId) { auto it = c.pois.find(poiId); return it == c.pois.end() ? poiId : it->second.name; }
static std::string itemLabel(const WorldContent& c, const std::string& id) { auto it = c.items.find(id); return it == c.items.end() ? id : it->second.label; }

void WorldState::reactNews(const EventEnvelope& e, std::vector<EventEnvelope>& out) {
    const Json& p = e.payload;
    const RegionState* r = region(e.region_id);
    const std::string regionName = r ? r->name : e.region_id;
    switch (e.event_type) {
        case EventType::MissionCompleted:
            out.push_back(newsEvent("\"" + p.get("title").asString() + "\" completed; " + regionName + " community acknowledges the operator", e));
            break;
        case EventType::CivilianReliefDelivered: {
            const EconomyNodeState* n = economyNode(p.get("node_id").asString());
            out.push_back(newsEvent(std::to_string(p.get("quantity").asInt()) + " x " + itemLabel(content_, p.get("item_id").asString()) + " delivered to " + (n ? poiNameOf(content_, n->poi_id) : p.get("node_id").asString()), e));
            break;
        }
        case EventType::InfrastructureStatusChanged: {
            auto it = content_.infrastructure.find(p.get("infrastructure_id").asString());
            std::string where = it == content_.infrastructure.end() ? p.get("infrastructure_id").asString() : poiNameOf(content_, it->second.poi_id);
            std::string kind = it == content_.infrastructure.end() ? "facility" : it->second.kind;
            out.push_back(newsEvent(kind + " services at " + where + " now " + p.get("new_status").asString(), e));
            break;
        }
        case EventType::WeatherChanged:
            out.push_back(newsEvent("Weather update for " + regionName + ": " + p.get("new").asString(), e));
            break;
        case EventType::RegionStabilityChanged: {
            double prev = p.get("previous").asNumber(), nv = p.get("new").asNumber();
            if (prev >= 0.5 && nv < 0.5) out.push_back(newsEvent("Civic stability concerns reported in " + regionName, e));
            else if (prev < 0.5 && nv >= 0.5) out.push_back(newsEvent("Civic stability improving in " + regionName, e));
            break;
        }
        case EventType::RegionControlChanged:
            out.push_back(newsEvent("Administration of " + regionName + " changes hands", e));
            break;
        case EventType::MissionGenerated:
            out.push_back(newsEvent("Relief request: " + p.get("template").get("title").asString(), e));
            break;
        case EventType::MissionFailed: {
            const MissionTemplate* d = missionDef(p.get("mission_id").asString());
            out.push_back(newsEvent(std::string(d ? d->title : p.get("mission_id").asString()) + (p.get("reason").asString() == "expired" ? " — request withdrawn" : " — deadline missed"), e));
            break;
        }
        case EventType::CampaignFlagChanged:
            if (p.get("flag").asString() == "act") out.push_back(newsEvent("A new chapter begins: " + p.get("new").asString(), e));
            break;
        case EventType::ResourceStockChanged: {
            const EconomyNodeState* n = economyNode(p.get("node_id").asString());
            if (!n) break;
            const std::string item = p.get("item_id").asString();
            auto tit = n->target_stock.find(item);
            int target = tit == n->target_stock.end() ? n->capacityOf(item) / 2 : tit->second;
            if (target <= 0) break;
            double prevShort = std::clamp((target - static_cast<double>(p.get("previous_quantity").asInt())) / target, 0.0, 1.0);
            double nowShort = n->shortage(item);
            if (prevShort < 0.8 && nowShort >= 0.8)
                out.push_back(newsEvent("Shortage of " + itemLabel(content_, item) + " reported at " + poiNameOf(content_, n->poi_id), e));
            break;
        }
        default: break;
    }
}

// ============================================================ projections
std::vector<MissionBoardEntry> WorldState::missionBoard() const {
    std::vector<MissionBoardEntry> board;
    std::vector<std::pair<std::string, const MissionTemplate*>> all;
    for (const auto& kv : content_.mission_templates) all.emplace_back(kv.first, &kv.second);
    for (const auto& kv : generated_) all.emplace_back(kv.first, &kv.second);
    for (const auto& kv : all) {
        MissionBoardEntry entry; entry.def = kv.second;
        const MissionState* st = mission(kv.first);
        if (st) entry.state = *st;
        std::string why;
        if (entry.state.status == MissionStatus::Available && !missionPrerequisitesMet(*kv.second, &why)) entry.blocked_reason = why;
        // Transport modes allowed right now, considering weather/infrastructure in the mission's region.
        const EntityId regionId = kv.second->region_id.empty() ? player_.region_id : kv.second->region_id;
        for (VehicleClass c : kv.second->eligible_vehicle_classes) if (checkTransportMode(c, regionId)) entry.allowed_vehicle_classes.push_back(c);
        board.push_back(entry);
    }
    return board;
}

std::string WorldState::npcDialogue(const EntityId& npcId) const {
    auto dit = content_.dialogue.find(npcId);
    auto pick = [&](const std::string& tag) -> std::string {
        if (dit == content_.dialogue.end()) return "";
        const RegionState* r = region(player_.region_id);
        if (r && r->local_trust >= 20) { auto t = dit->second.find(tag + "_trusted"); if (t != dit->second.end()) return t->second; }
        auto t = dit->second.find(tag); return t == dit->second.end() ? "" : t->second;
    };
    std::string chosen;
    bool anyOffered = false, allCompleted = true;
    for (const auto& kv : content_.mission_templates) {
        if (kv.second.offeringNpc() != npcId) continue;
        anyOffered = true;
        const MissionState* st = mission(kv.first);
        if (!st) continue;
        if (st->status != MissionStatus::Completed) allCompleted = false;
        if (st->status == MissionStatus::Active) {
            if (st->current_objective_index < static_cast<int>(kv.second.objectives.size())) {
                const ObjectiveDef& o = kv.second.objectives[static_cast<size_t>(st->current_objective_index)];
                chosen = (o.type == ObjectiveType::Interact && o.target_id == npcId) ? pick("debrief_ready") : pick("in_progress");
            }
        } else if (st->status == MissionStatus::Available && chosen.empty()) {
            std::string why; chosen = missionPrerequisitesMet(kv.second, &why) ? pick("offer") : pick("locked");
        }
    }
    if (chosen.empty() && anyOffered && allCompleted) chosen = pick("completed");
    if (chosen.empty()) chosen = pick("default");
    return chosen;
}

std::string WorldState::debugSummary() const {
    std::string s;
    char buf[256];
    std::snprintf(buf, sizeof buf, "tick=%lld day=%d %02d:%02d | journal=%zu seq=%llu | news=%zu\n",
                  static_cast<long long>(time_.tick), time_.day(), time_.hour(), time_.minute(), journal_.size(),
                  static_cast<unsigned long long>(journal_.sequence()), news_.size());
    s += buf;
    Pose pp = playerEffectivePose();
    std::snprintf(buf, sizeof buf, "player=%s credits=%d region=%s pos=(%.1f,%.1f,%.1f) mode=%s vehicle=%s seat=%s role=%s\n",
                  player_.entity_id.c_str(), player_.credits, player_.region_id.c_str(), pp.position.x, pp.position.y, pp.position.z,
                  possession_.mode == PossessionMode::Seated ? "seated" : "on_foot", possession_.vehicle_id.c_str(), possession_.seat_id.c_str(), possession_.role.c_str());
    s += buf;
    for (const auto& kv : regions_) {
        const WeatherState* w = regionWeather(kv.first);
        std::snprintf(buf, sizeof buf, "region %s control=%s stability=%.3f trust=%d(%s) weather=%s\n", kv.first.c_str(), kv.second.control_faction_id.c_str(),
                      kv.second.stability_normalized, kv.second.local_trust, reputationBand(kv.second.local_trust), w ? w->condition.c_str() : "?");
        s += buf;
    }
    for (const auto& kv : factions_) { std::snprintf(buf, sizeof buf, "faction %s rep=%d(%s)\n", kv.first.c_str(), kv.second.reputation, reputationBand(kv.second.reputation)); s += buf; }
    for (const auto& kv : infra_) { std::snprintf(buf, sizeof buf, "infra %s %s\n", kv.first.c_str(), kv.second.status.c_str()); s += buf; }
    for (const auto& kv : economy_) {
        s += "node " + kv.first + ":";
        for (const auto& st : kv.second.stocks) { std::snprintf(buf, sizeof buf, " %s=%d(short %.2f)", st.first.c_str(), st.second, kv.second.shortage(st.first)); s += buf; }
        s += "\n";
    }
    for (const auto& kv : vehicles_) {
        std::snprintf(buf, sizeof buf, "vehicle %s def=%s pos=(%.1f,%.1f,%.1f) cond=%.2f seats=%zu cargo=%d\n", kv.first.c_str(), kv.second.definition_id.c_str(),
                      kv.second.pose.position.x, kv.second.pose.position.y, kv.second.pose.position.z, kv.second.condition_normalized, kv.second.occupied_seats.size(), kv.second.cargoUnits(content_));
        s += buf;
    }
    for (const auto& kv : missions_) {
        std::snprintf(buf, sizeof buf, "mission %s %s obj=%d delivered=%d\n", kv.first.c_str(), missionStatusName(kv.second.status), kv.second.current_objective_index, kv.second.delivered_quantity);
        s += buf;
    }
    s += "last events:\n";
    for (const auto& e : journal_.lastN(20)) { std::snprintf(buf, sizeof buf, "  %s t=%lld %s <- %s\n", e.event_id.c_str(), static_cast<long long>(e.occurred_at_world_tick), eventTypeName(e.event_type), e.source_entity_id.c_str()); s += buf; }
    return s;
}

} // namespace ef
