#include "EFWorldCore/ContentLoader.h"
#include <fstream>
#include <sstream>

namespace ef {

static std::map<std::string, int> mapInt(const Json& j) { std::map<std::string, int> m; for (const auto& kv : j.asObject()) m[kv.first] = static_cast<int>(kv.second.asInt()); return m; }
static std::vector<std::string> strList(const Json& j) { std::vector<std::string> v; for (const auto& s : j.asArray()) v.push_back(s.asString()); return v; }

Status loadContentFromJson(const Json& j, WorldContent& c) {
    c = WorldContent{};
    if (!j.isObject()) return Status::Fail("content_invalid", "root is not an object");
    c.content_id = j.get("content_id").asString();
    if (c.content_id.empty()) return Status::Fail("content_invalid", "content_id missing");
    int schema = static_cast<int>(j.get("schema_version").asInt(1));
    if (schema != 1) return Status::Fail("content_invalid", "unsupported content schema_version");
    const Json& w = j.get("world");
    c.simulation_seed = static_cast<uint64_t>(w.get("simulation_seed").asInt(1));
    c.starting_region_id = w.get("starting_region").asString();
    c.starting_poi_id = w.get("starting_poi").asString();
    c.starting_credits = static_cast<int>(w.get("starting_credits").asInt(0));

    for (const auto& ij : j.get("items").asArray()) {
        ItemDef d; d.id = ij.get("id").asString(); d.label = ij.get("label").asString(); d.category = ij.get("category").asString();
        d.base_price_credits = static_cast<int>(ij.get("base_price_credits").asInt()); d.mass_game_units = static_cast<int>(ij.get("mass_game_units").asInt(1));
        if (d.id.empty()) return Status::Fail("content_invalid", "item without id");
        c.items[d.id] = d;
    }
    for (const auto& pj : j.get("pois").asArray()) {
        PoiDef p; p.id = pj.get("id").asString(); p.name = pj.get("name").asString(); p.kind = pj.get("kind").asString();
        p.region_id = pj.get("region_id").asString(); p.pose = Pose::fromJson(pj.get("pose"));
        if (p.id.empty()) return Status::Fail("content_invalid", "poi without id");
        c.pois[p.id] = p;
    }
    for (const auto& nj : j.get("npcs").asArray()) {
        NpcDef n; n.id = nj.get("id").asString(); n.name = nj.get("name").asString(); n.role_tag = nj.get("role_tag").asString();
        n.poi_id = nj.get("poi_id").asString(); n.faction_id = nj.get("faction_id").asString();
        if (!c.pois.count(n.poi_id)) return Status::Fail("content_invalid", "npc " + n.id + " references unknown poi " + n.poi_id);
        c.npcs[n.id] = n;
    }
    for (const auto& dj : j.get("vehicle_definitions").asArray()) {
        VehicleDefinition d; d.definition_id = dj.get("definition_id").asString(); d.display_name = dj.get("display_name").asString();
        d.control_profile = dj.get("control_profile").asString(); d.vehicle_class = vehicleClassFromName(dj.get("vehicle_class").asString());
        d.cargo_capacity_game_units = static_cast<int>(dj.get("cargo_capacity_game_units").asInt());
        if (d.vehicle_class == VehicleClass::Unknown) return Status::Fail("content_invalid", "vehicle definition " + d.definition_id + " has unknown class");
        d.realism_category = dj.get("realism_category").asString().empty() ? defaultRealismCategory(d.vehicle_class) : dj.get("realism_category").asString();
        d.handling = dj.get("handling").isObject() ? dj.get("handling") : Json::object();
        for (const auto& sj : dj.get("seats").asArray()) {
            SeatDef s;
            if (sj.isString()) { s.seat_id = sj.asString(); s.is_control_seat = (s.seat_id == "driver" || s.seat_id == "pilot" || s.seat_id == "captain"); }
            else { s.seat_id = sj.get("seat_id").asString(); s.is_control_seat = sj.get("is_control_seat").asBool(false); }
            d.seats.push_back(s);
        }
        c.vehicle_definitions[d.definition_id] = d;
    }
    for (const auto& ij : j.get("infrastructure").asArray()) {
        InfrastructureDef d; d.id = ij.get("id").asString(); d.kind = ij.get("kind").asString(); d.poi_id = ij.get("poi_id").asString(); d.region_id = ij.get("region_id").asString();
        if (!c.pois.count(d.poi_id)) return Status::Fail("content_invalid", "infrastructure " + d.id + " references unknown poi");
        c.infrastructure[d.id] = d;
    }
    for (const auto& mj : j.get("mission_templates").asArray()) {
        MissionTemplate m; m.id = mj.get("id").asString(); m.title = mj.get("title").asString(); m.type = mj.get("type").asString();
        m.issuer_faction_id = mj.get("issuer_faction_id").asString(); m.region_id = mj.get("region_id").asString();
        for (const auto& v : mj.get("eligible_vehicle_classes").asArray()) m.eligible_vehicle_classes.push_back(vehicleClassFromName(v.asString()));
        for (const auto& oj : mj.get("objectives").asArray()) {
            ObjectiveDef o; o.id = oj.get("id").asString(); o.type = objectiveTypeFromName(oj.get("type").asString());
            o.target_id = oj.get("target_id").asString(); o.item_id = oj.get("item_id").asString();
            o.quantity = static_cast<int>(oj.get("quantity").asInt()); o.destination_node_id = oj.get("destination_node_id").asString();
            if (o.type == ObjectiveType::Unknown) return Status::Fail("content_invalid", "mission " + m.id + " objective " + o.id + " has unknown type");
            if (o.type == ObjectiveType::Interact && !c.npcs.count(o.target_id)) return Status::Fail("content_invalid", "mission " + m.id + " targets unknown npc " + o.target_id);
            m.objectives.push_back(o);
        }
        const Json& r = mj.get("rewards");
        m.rewards.credits = static_cast<int>(r.get("credits").asInt()); m.rewards.local_trust_delta = static_cast<int>(r.get("local_trust_delta").asInt());
        m.rewards.faction_reputation_delta = static_cast<int>(r.get("faction_reputation_delta").asInt());
        const Json& pre = mj.get("prerequisites");
        m.min_local_trust = static_cast<int>(pre.get("min_local_trust").asInt(-100));
        m.required_completed_missions = strList(pre.get("required_completed_missions"));
        if (m.objectives.empty()) return Status::Fail("content_invalid", "mission " + m.id + " has no objectives");
        c.mission_templates[m.id] = m;
    }
    for (const auto& kv : j.get("dialogue").asObject())
        for (const auto& line : kv.second.asObject()) c.dialogue[kv.first][line.first] = line.second.asString();

    for (const auto& rj : j.get("regions").asArray()) {
        RegionState r; r.id = rj.get("id").asString(); r.name = rj.get("name").asString(); r.control_faction_id = rj.get("control_faction_id").asString();
        r.stability_normalized = rj.get("stability_normalized").asNumber(0.5); r.population_index = static_cast<int>(rj.get("population_index").asInt());
        r.local_trust = static_cast<int>(rj.get("initial_local_trust").asInt(0)); r.spawn_poi_id = rj.get("spawn_poi_id").asString();
        r.economy_node_ids = strList(rj.get("economy_node_ids")); r.poi_ids = strList(rj.get("poi_ids"));
        c.initial_regions.push_back(r);
    }
    for (const auto& fj : j.get("factions").asArray()) {
        FactionState f; f.id = fj.get("id").asString(); f.name = fj.get("name").asString(); f.reputation = static_cast<int>(fj.get("initial_relation_to_player").asInt());
        c.initial_factions.push_back(f);
    }
    for (const auto& nj : j.get("economy_nodes").asArray()) {
        EconomyNodeState n; n.id = nj.get("id").asString(); n.region_id = nj.get("region_id").asString(); n.poi_id = nj.get("poi_id").asString();
        n.stocks = mapInt(nj.get("stocks")); n.capacity = mapInt(nj.get("capacity_by_item")); n.target_stock = mapInt(nj.get("target_stock"));
        n.production_per_step = mapInt(nj.get("production_per_step")); n.consumption_per_step = mapInt(nj.get("consumption_per_step"));
        if (!c.pois.count(n.poi_id)) return Status::Fail("content_invalid", "economy node " + n.id + " references unknown poi " + n.poi_id);
        c.initial_economy_nodes.push_back(n);
    }
    for (const auto& vj : j.get("vehicles").asArray()) {
        VehicleState v; v.entity_id = vj.get("id").asString(); v.definition_id = vj.get("definition_id").asString();
        v.region_id = vj.get("region_id").asString(); v.home_poi_id = vj.get("home_poi_id").asString();
        v.condition_normalized = vj.get("condition_normalized").asNumber(1.0); v.fuel_normalized = vj.get("fuel_normalized").asNumber(1.0);
        if (vj.has("pose")) v.pose = Pose::fromJson(vj.get("pose"));
        else if (c.pois.count(v.home_poi_id)) v.pose = c.pois[v.home_poi_id].pose;
        if (!c.vehicle_definitions.count(v.definition_id)) return Status::Fail("content_invalid", "vehicle " + v.entity_id + " unknown definition " + v.definition_id);
        c.initial_vehicles.push_back(v);
    }
    if (!c.pois.count(c.starting_poi_id)) return Status::Fail("content_invalid", "world.starting_poi unknown");
    return Status::Ok();
}

Status loadContentFromString(const std::string& text, WorldContent& out) {
    Json j; std::string err;
    if (!Json::parse(text, j, &err)) return Status::Fail("content_parse_error", err);
    return loadContentFromJson(j, out);
}

Status loadContentFromFile(const std::string& path, WorldContent& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return Status::Fail("content_file_missing", path);
    std::stringstream ss; ss << in.rdbuf();
    return loadContentFromString(ss.str(), out);
}

} // namespace ef
