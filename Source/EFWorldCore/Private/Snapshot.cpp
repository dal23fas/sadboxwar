// Snapshot serialization (master prompt §36, TDD §12). One schema, one world clock.
#include "EFWorldCore/WorldState.h"

namespace ef {

static Json mapIntToJson(const std::map<std::string, int>& m) { Json j = Json::object(); for (const auto& kv : m) j.set(kv.first, kv.second); return j; }
static std::map<std::string, int> jsonToMapInt(const Json& j) { std::map<std::string, int> m; for (const auto& kv : j.asObject()) m[kv.first] = static_cast<int>(kv.second.asInt()); return m; }
static Json strListToJson(const std::vector<std::string>& v) { Json a = Json::array(); for (const auto& s : v) a.push(s); return a; }
static std::vector<std::string> jsonToStrList(const Json& j) { std::vector<std::string> v; for (const auto& s : j.asArray()) v.push_back(s.asString()); return v; }

Json WorldState::toSnapshot() const {
    Json s = Json::object();
    s.set("schema_version", kSaveSchemaVersion);
    s.set("content_id", content_.content_id);
    s.set("simulation_seed", static_cast<uint64_t>(seed_));
    Json t = Json::object(); t.set("tick", static_cast<int64_t>(time_.tick)); t.set("ticks_per_hour", time_.ticks_per_hour); t.set("start_hour", time_.start_hour);
    s.set("time", t);

    Json pl = Json::object();
    pl.set("entity_id", player_.entity_id); pl.set("display_name", player_.display_name); pl.set("region_id", player_.region_id);
    pl.set("pose", player_.pose.toJson()); pl.set("credits", player_.credits); pl.set("carry_capacity_game_units", player_.carry_capacity_game_units);
    pl.set("inventory", mapIntToJson(player_.inventory));
    Json q = Json::array(); for (const auto& x : player_.qualifications) q.push(x); pl.set("qualifications", q);
    pl.set("career_history", strListToJson(player_.career_history));
    s.set("player", pl);

    Json po = Json::object();
    po.set("mode", possession_.mode == PossessionMode::Seated ? "seated" : "on_foot");
    po.set("vehicle_id", possession_.vehicle_id); po.set("seat_id", possession_.seat_id); po.set("role", possession_.role);
    s.set("possession", po);

    Json ca = Json::object(); ca.set("act", campaign_.act);
    Json fl = Json::object(); for (const auto& kv : campaign_.flags) fl.set(kv.first, kv.second); ca.set("flags", fl);
    s.set("campaign", ca);

    Json regions = Json::array();
    for (const auto& kv : regions_) {
        const RegionState& r = kv.second; Json j = Json::object();
        j.set("id", r.id); j.set("name", r.name); j.set("control_faction_id", r.control_faction_id); j.set("stability_normalized", r.stability_normalized);
        j.set("population_index", r.population_index); j.set("local_trust", r.local_trust); j.set("spawn_poi_id", r.spawn_poi_id);
        j.set("economy_node_ids", strListToJson(r.economy_node_ids)); j.set("poi_ids", strListToJson(r.poi_ids));
        regions.push(j);
    }
    s.set("regions", regions);

    Json factions = Json::array();
    for (const auto& kv : factions_) { Json j = Json::object(); j.set("id", kv.second.id); j.set("name", kv.second.name); j.set("reputation", kv.second.reputation); factions.push(j); }
    s.set("factions", factions);

    Json nodes = Json::array();
    for (const auto& kv : economy_) {
        const EconomyNodeState& n = kv.second; Json j = Json::object();
        j.set("id", n.id); j.set("region_id", n.region_id); j.set("poi_id", n.poi_id);
        j.set("stocks", mapIntToJson(n.stocks)); j.set("capacity_by_item", mapIntToJson(n.capacity)); j.set("target_stock", mapIntToJson(n.target_stock));
        j.set("production_per_step", mapIntToJson(n.production_per_step)); j.set("consumption_per_step", mapIntToJson(n.consumption_per_step));
        nodes.push(j);
    }
    s.set("economy_nodes", nodes);

    Json vehicles = Json::array();
    for (const auto& kv : vehicles_) {
        const VehicleState& v = kv.second; Json j = Json::object();
        j.set("entity_id", v.entity_id); j.set("definition_id", v.definition_id); j.set("region_id", v.region_id); j.set("home_poi_id", v.home_poi_id);
        j.set("pose", v.pose.toJson()); j.set("condition_normalized", v.condition_normalized); j.set("fuel_normalized", v.fuel_normalized);
        j.set("cargo", mapIntToJson(v.cargo));
        Json seats = Json::array();
        for (const auto& sk : v.occupied_seats) { Json sj = Json::object(); sj.set("seat_id", sk.first); sj.set("occupant_entity_id", sk.second); seats.push(sj); }
        j.set("occupied_seats", seats);
        vehicles.push(j);
    }
    s.set("vehicles", vehicles);

    Json missions = Json::array();
    for (const auto& kv : missions_) {
        const MissionState& m = kv.second; Json j = Json::object();
        j.set("mission_id", m.mission_id); j.set("status", missionStatusName(m.status)); j.set("current_objective_index", m.current_objective_index);
        j.set("delivered_quantity", m.delivered_quantity); j.set("accepted_tick", static_cast<int64_t>(m.accepted_tick));
        j.set("completed_tick", static_cast<int64_t>(m.completed_tick)); j.set("completion_event_id", m.completion_event_id);
        missions.push(j);
    }
    s.set("missions", missions);
    Json gen = Json::array(); for (const auto& kv : generated_) gen.push(kv.second.toJson()); s.set("generated_missions", gen);

    Json infra = Json::array();
    for (const auto& kv : infra_) { Json j = Json::object(); j.set("id", kv.second.id); j.set("status", kv.second.status); infra.push(j); }
    s.set("infrastructure", infra);

    Json weather = Json::array();
    for (const auto& kv : weather_) { Json j = Json::object(); j.set("region_id", kv.second.region_id); j.set("condition", kv.second.condition); j.set("last_change_tick", static_cast<int64_t>(kv.second.last_change_tick)); weather.push(j); }
    s.set("weather", weather);

    Json news = Json::array();
    for (const auto& n : news_) { Json j = Json::object(); j.set("news_id", n.news_id); j.set("headline", n.headline); j.set("tick", static_cast<int64_t>(n.tick)); j.set("source_event_id", n.source_event_id); j.set("region_id", n.region_id); news.push(j); }
    s.set("news", news);

    s.set("journal", journal_.toJson());
    return s;
}

Status WorldState::restoreSnapshot(const Json& s, std::vector<std::string>* warnings) {
    auto warn = [&](const std::string& w) { if (warnings) warnings->push_back(w); };
    if (!initialized_) return Status::Fail("not_initialized", "load content first, then restore the snapshot");
    if (!s.isObject()) return Status::Fail("save_corrupt", "snapshot is not an object");
    int schema = static_cast<int>(s.get("schema_version").asInt(-1));
    if (schema < 1 || schema > kSaveSchemaVersion) return Status::Fail("save_schema_unsupported", "schema_version " + std::to_string(schema));
    // Migration table: v1 is current. Future: if (schema == 1) migrate_1_to_2(s) ...
    if (s.get("content_id").asString() != content_.content_id)
        warn("content_id mismatch: save=" + s.get("content_id").asString() + " loaded=" + content_.content_id);

    // Start from a clean StartNewGame state, then overlay the snapshot.
    WorldContent keep = content_;
    Status init = initFromContent(keep);
    if (!init) return init;

    seed_ = static_cast<uint64_t>(s.get("simulation_seed").asInt(static_cast<int64_t>(seed_)));
    time_.tick = s.get("time").get("tick").asInt(0);
    time_.ticks_per_hour = static_cast<int>(s.get("time").get("ticks_per_hour").asInt(60));
    time_.start_hour = static_cast<int>(s.get("time").get("start_hour").asInt(8));

    const Json& pl = s.get("player");
    if (pl.isObject()) {
        player_.entity_id = pl.get("entity_id").asString().empty() ? player_.entity_id : pl.get("entity_id").asString();
        player_.display_name = pl.get("display_name").asString();
        player_.region_id = pl.get("region_id").asString();
        player_.pose = Pose::fromJson(pl.get("pose"));
        player_.credits = static_cast<int>(pl.get("credits").asInt());
        player_.carry_capacity_game_units = static_cast<int>(pl.get("carry_capacity_game_units").asInt(12));
        player_.inventory = jsonToMapInt(pl.get("inventory"));
        player_.qualifications.clear(); for (const auto& q : pl.get("qualifications").asArray()) player_.qualifications.insert(q.asString());
        player_.career_history = jsonToStrList(pl.get("career_history"));
    }
    const Json& ca = s.get("campaign");
    campaign_.act = ca.get("act").asString().empty() ? campaign_.act : ca.get("act").asString();
    campaign_.flags.clear(); for (const auto& kv : ca.get("flags").asObject()) campaign_.flags[kv.first] = kv.second.asString();

    for (const auto& j : s.get("regions").asArray()) {
        auto it = regions_.find(j.get("id").asString());
        if (it == regions_.end()) { warn("unknown region in save: " + j.get("id").asString()); continue; }
        it->second.control_faction_id = j.get("control_faction_id").asString();
        it->second.stability_normalized = j.get("stability_normalized").asNumber(it->second.stability_normalized);
        it->second.population_index = static_cast<int>(j.get("population_index").asInt(it->second.population_index));
        it->second.local_trust = static_cast<int>(j.get("local_trust").asInt());
    }
    for (const auto& j : s.get("factions").asArray()) {
        auto it = factions_.find(j.get("id").asString());
        if (it == factions_.end()) { warn("unknown faction in save: " + j.get("id").asString()); continue; }
        it->second.reputation = static_cast<int>(j.get("reputation").asInt());
    }
    for (const auto& j : s.get("economy_nodes").asArray()) {
        auto it = economy_.find(j.get("id").asString());
        if (it == economy_.end()) { warn("unknown economy node in save: " + j.get("id").asString()); continue; }
        it->second.stocks = jsonToMapInt(j.get("stocks"));
        if (j.has("capacity_by_item")) it->second.capacity = jsonToMapInt(j.get("capacity_by_item"));
        if (j.has("target_stock")) it->second.target_stock = jsonToMapInt(j.get("target_stock"));
        if (j.has("production_per_step")) it->second.production_per_step = jsonToMapInt(j.get("production_per_step"));
        if (j.has("consumption_per_step")) it->second.consumption_per_step = jsonToMapInt(j.get("consumption_per_step"));
    }
    for (const auto& j : s.get("vehicles").asArray()) {
        const std::string id = j.get("entity_id").asString();
        VehicleState v;
        auto it = vehicles_.find(id);
        if (it != vehicles_.end()) v = it->second;
        v.entity_id = id; v.definition_id = j.get("definition_id").asString();
        if (!content_.vehicle_definitions.count(v.definition_id)) { warn("vehicle " + id + " has unknown definition " + v.definition_id + "; kept as placeholder"); }
        v.region_id = j.get("region_id").asString(); v.home_poi_id = j.get("home_poi_id").asString();
        v.pose = Pose::fromJson(j.get("pose")); v.condition_normalized = j.get("condition_normalized").asNumber(1.0); v.fuel_normalized = j.get("fuel_normalized").asNumber(1.0);
        v.cargo = jsonToMapInt(j.get("cargo"));
        v.occupied_seats.clear();
        for (const auto& sj : j.get("occupied_seats").asArray()) v.occupied_seats[sj.get("seat_id").asString()] = sj.get("occupant_entity_id").asString();
        vehicles_[id] = v;
    }
    for (const auto& j : s.get("generated_missions").asArray()) {
        MissionTemplate t; std::string err;
        if (!MissionTemplate::fromJson(j, t, &err)) { warn("generated mission dropped: " + err); continue; }
        t.generated = true; generated_[t.id] = t;
        if (!missions_.count(t.id)) { MissionState m; m.mission_id = t.id; missions_[t.id] = m; }
    }
    for (const auto& j : s.get("missions").asArray()) {
        auto it = missions_.find(j.get("mission_id").asString());
        if (it == missions_.end()) { warn("unknown mission in save (dropped): " + j.get("mission_id").asString()); continue; }
        it->second.status = missionStatusFromName(j.get("status").asString());
        it->second.current_objective_index = static_cast<int>(j.get("current_objective_index").asInt());
        it->second.delivered_quantity = static_cast<int>(j.get("delivered_quantity").asInt());
        it->second.accepted_tick = j.get("accepted_tick").asInt(-1); it->second.completed_tick = j.get("completed_tick").asInt(-1);
        it->second.completion_event_id = j.get("completion_event_id").asString();
    }
    for (const auto& j : s.get("infrastructure").asArray()) {
        auto it = infra_.find(j.get("id").asString());
        if (it == infra_.end()) { warn("unknown infrastructure in save: " + j.get("id").asString()); continue; }
        it->second.status = j.get("status").asString();
    }
    for (const auto& j : s.get("weather").asArray()) {
        auto it = weather_.find(j.get("region_id").asString());
        if (it == weather_.end()) continue;
        it->second.condition = j.get("condition").asString(); it->second.last_change_tick = j.get("last_change_tick").asInt(0);
    }
    news_.clear();
    for (const auto& j : s.get("news").asArray()) {
        NewsItem n; n.news_id = j.get("news_id").asString(); n.headline = j.get("headline").asString(); n.tick = j.get("tick").asInt();
        n.source_event_id = j.get("source_event_id").asString(); n.region_id = j.get("region_id").asString(); news_.push_back(n);
    }
    std::string jerr;
    if (!journal_.fromJson(s.get("journal"), &jerr)) return Status::Fail("save_corrupt", "journal: " + jerr);

    // Possession with validation + safe fallback (SAVE-02, TDD §5).
    const Json& po = s.get("possession");
    possession_ = PossessionState{};
    possession_.role = po.get("role").asString().empty() ? "infantry" : po.get("role").asString();
    if (po.get("mode").asString() == "seated") {
        const std::string vid = po.get("vehicle_id").asString(), sid = po.get("seat_id").asString();
        const VehicleDefinition* d = vehicleDef(vid);
        const VehicleState* v = vehicle(vid);
        bool seatOk = v && d && d->hasSeat(sid) && v->occupied_seats.count(sid) && v->occupied_seats.at(sid) == player_.entity_id;
        if (seatOk) { possession_.mode = PossessionMode::Seated; possession_.vehicle_id = vid; possession_.seat_id = sid; }
        else {
            warn("seated possession invalid (vehicle=" + vid + " seat=" + sid + "); fell back to on-foot at region spawn");
            possession_.role = "infantry";
            for (auto& vv : vehicles_) {   // clear any stale seat occupancy by the player
                for (auto it = vv.second.occupied_seats.begin(); it != vv.second.occupied_seats.end();) {
                    if (it->second == player_.entity_id) it = vv.second.occupied_seats.erase(it); else ++it;
                }
            }
            const RegionState* r = region(player_.region_id);
            const PoiDef* spawn = r && !r->spawn_poi_id.empty() ? poi(r->spawn_poi_id) : poi(content_.starting_poi_id);
            if (spawn) player_.pose = spawn->pose;
        }
    }
    if (!regions_.count(player_.region_id)) { warn("player region unknown; reset to starting region"); player_.region_id = content_.starting_region_id; }
    return Status::Ok();
}

} // namespace ef
