#include "EFWorldCore/JsonApi.h"
#include "EFWorldCore/ContentLoader.h"
#include <cstdio>
#include <filesystem>
#include <chrono>

namespace ef {

JsonApi::JsonApi(std::string saveDir) : saveDir_(std::move(saveDir)) {
    storage_ = std::make_unique<FileSaveStorage>(saveDir_);
    settingsStorage_ = std::make_unique<FileSaveStorage>(saveDir_ + "/settings");
    saves_ = std::make_unique<SaveService>(*storage_);
}

static const char* kRealismSettingsKey = "realism.json";

Json JsonApi::handleRealism(const Json& id, const std::string& op, const Json& a) {
    if (op == "load_presets") {
        Status s;
        if (a.has("presets_text")) { Json pj; std::string err; if (!Json::parse(a.get("presets_text").asString(), pj, &err)) s = Status::Fail("presets_parse_error", err); else s = realism_.loadPresets(pj); }
        else s = realism_.loadPresetsFromFile(a.get("path").asString());
        if (!s) return fail(id, s.code, s.message);
        // Auto-load previously saved settings if present (settings persist independently of world saves).
        std::string bytes;
        if (settingsStorage_->exists(kRealismSettingsKey) && settingsStorage_->read(kRealismSettingsKey, bytes).ok) {
            Json j; if (Json::parse(bytes, j)) realism_.settingsFromJson(j);
        }
        return ok(id, realism_.fullProjection());
    }
    if (!realism_.loaded()) return fail(id, "presets_not_loaded", "call load_presets first");
    Status s;
    if (op == "realism_get") return ok(id, realism_.fullProjection());
    else if (op == "realism_set_global") s = realism_.setGlobalPreset(a.get("preset").asString());
    else if (op == "realism_set_category") s = realism_.setCategoryPreset(a.get("category").asString(), a.get("preset").asString());
    else if (op == "realism_set_override") s = realism_.setOverride(a.get("category").asString(), a.get("parameter").asString(), static_cast<int>(a.get("value").asInt(-1)));
    else if (op == "realism_reset") { if (a.get("category").asString().empty()) realism_.resetAll(); else s = realism_.resetCategory(a.get("category").asString()); }
    else if (op == "realism_set_combat") s = realism_.setCombatChallenge(a.get("level").asString());
    else if (op == "realism_set_hud") s = realism_.setHudAssist(a.get("level").asString());
    else if (op == "realism_set_accessibility") s = realism_.setAccessibility(a.get("key").asString(), a.get("value"));
    else if (op == "realism_save") s = settingsStorage_->writeAtomic(kRealismSettingsKey, realism_.settingsToJson().dump(2));
    else if (op == "realism_load") {
        std::string bytes; s = settingsStorage_->read(kRealismSettingsKey, bytes);
        if (s) { Json j; std::string err; if (!Json::parse(bytes, j, &err)) s = Status::Fail("settings_corrupt", err); else s = realism_.settingsFromJson(j); }
    }
    else return fail(id, "unknown_op", op);
    if (!s) return fail(id, s.code, s.message);
    return ok(id, realism_.fullProjection());
}

Json JsonApi::ok(const Json& id, Json result) const { Json r = Json::object(); r.set("id", id); r.set("ok", true); r.set("result", std::move(result)); return r; }
Json JsonApi::fail(const Json& id, const std::string& code, const std::string& message) const {
    Json r = Json::object(); r.set("id", id); r.set("ok", false); r.set("code", code); r.set("message", message); return r;
}

Status JsonApi::commandFromJson(const Json& j, Command& out) {
    const std::string type = j.get("type").asString();
    auto str = [&](const char* k) { return j.get(k).asString(); };
    auto num = [&](const char* k) { return static_cast<int>(j.get(k).asInt()); };
    if (type == "InteractNpc") out = CmdInteractNpc{str("npc_id")};
    else if (type == "AcceptMission") out = CmdAcceptMission{str("mission_id")};
    else if (type == "EnterVehicle") out = CmdEnterVehicle{str("vehicle_id"), str("seat_id")};
    else if (type == "ExitVehicle") out = CmdExitVehicle{};
    else if (type == "SwitchSeat") out = CmdSwitchSeat{str("seat_id")};
    else if (type == "ReportPose") out = CmdReportPose{str("entity_id"), Pose::fromJson(j.get("pose")), j.get("altitude_agl").asNumber(-1.0)};
    else if (type == "LoadCargo") out = CmdLoadCargo{str("node_id"), str("item_id"), num("quantity")};
    else if (type == "DeliverCargo") out = CmdDeliverCargo{str("node_id"), str("item_id"), num("quantity")};
    else if (type == "SetVehicleCondition") out = CmdSetVehicleCondition{str("vehicle_id"), j.get("condition_normalized").asNumber(1.0), str("reason_tag")};
    else if (type == "SetWeather") out = CmdSetWeather{str("region_id"), str("condition")};
    else if (type == "SetCampaignFlag") out = CmdSetCampaignFlag{str("flag"), str("value")};
    else if (type == "RefuelVehicle") out = CmdRefuelVehicle{str("vehicle_id")};
    else if (type == "RepairVehicle") out = CmdRepairVehicle{str("vehicle_id")};
    else return Status::Fail("unknown_command", type);
    return Status::Ok();
}

static Json mapIntJson(const std::map<std::string, int>& m) { Json j = Json::object(); for (const auto& kv : m) j.set(kv.first, kv.second); return j; }

Json JsonApi::contentProjection() const {
    const WorldContent& c = world_.content();
    Json r = Json::object();
    r.set("content_id", c.content_id);
    r.set("starting_poi", c.starting_poi_id);
    Json pois = Json::array();
    for (const auto& kv : c.pois) { Json p = Json::object(); p.set("id", kv.first); p.set("name", kv.second.name); p.set("kind", kv.second.kind); p.set("region_id", kv.second.region_id); p.set("pose", kv.second.pose.toJson()); pois.push(p); }
    r.set("pois", pois);
    Json npcs = Json::array();
    for (const auto& kv : c.npcs) { Json n = Json::object(); n.set("id", kv.first); n.set("name", kv.second.name); n.set("role_tag", kv.second.role_tag); n.set("poi_id", kv.second.poi_id); n.set("faction_id", kv.second.faction_id); auto p = c.pois.find(kv.second.poi_id); n.set("pose", p == c.pois.end() ? Json() : p->second.pose.toJson()); npcs.push(n); }
    r.set("npcs", npcs);
    Json defs = Json::array();
    for (const auto& kv : c.vehicle_definitions) {
        Json d = Json::object(); d.set("definition_id", kv.first); d.set("display_name", kv.second.display_name); d.set("vehicle_class", vehicleClassName(kv.second.vehicle_class));
        d.set("control_profile", kv.second.control_profile); d.set("cargo_capacity_game_units", kv.second.cargo_capacity_game_units);
        d.set("realism_category", kv.second.realism_category); d.set("handling", kv.second.handling);
        Json seats = Json::array(); for (const auto& s : kv.second.seats) { Json sj = Json::object(); sj.set("seat_id", s.seat_id); sj.set("is_control_seat", s.is_control_seat); seats.push(sj); }
        d.set("seats", seats); defs.push(d);
    }
    r.set("vehicle_definitions", defs);
    Json items = Json::array();
    for (const auto& kv : c.items) { Json i = Json::object(); i.set("id", kv.first); i.set("label", kv.second.label); i.set("category", kv.second.category); i.set("base_price_credits", kv.second.base_price_credits); i.set("mass_game_units", kv.second.mass_game_units); items.push(i); }
    r.set("items", items);
    Json nodes = Json::array();
    for (const auto& kv : world_.economyNodes()) { Json n = Json::object(); n.set("id", kv.first); n.set("poi_id", kv.second.poi_id); n.set("region_id", kv.second.region_id); nodes.push(n); }
    r.set("economy_nodes", nodes);
    return r;
}

Json JsonApi::missionBoardProjection() const {
    Json arr = Json::array();
    for (const auto& e : world_.missionBoard()) {
        Json m = Json::object();
        m.set("mission_id", e.def->id); m.set("title", e.def->title); m.set("type", e.def->type); m.set("issuer_faction_id", e.def->issuer_faction_id);
        m.set("status", missionStatusName(e.state.status)); m.set("current_objective_index", e.state.current_objective_index);
        m.set("delivered_quantity", e.state.delivered_quantity); m.set("blocked_reason", e.blocked_reason);
        m.set("offering_npc_id", e.def->offeringNpc()); m.set("generated", e.def->generated); m.set("region_id", e.def->region_id);
        m.set("time_limit_ticks", static_cast<int64_t>(e.def->time_limit_ticks)); m.set("accepted_tick", static_cast<int64_t>(e.state.accepted_tick));
        Json allowed = Json::array(); for (VehicleClass c : e.allowed_vehicle_classes) allowed.push(vehicleClassName(c)); m.set("allowed_vehicle_classes", allowed);
        Json objs = Json::array();
        for (size_t i = 0; i < e.def->objectives.size(); ++i) {
            const ObjectiveDef& o = e.def->objectives[i]; Json oj = Json::object();
            oj.set("id", o.id); oj.set("type", objectiveTypeName(o.type)); oj.set("target_id", o.target_id); oj.set("item_id", o.item_id);
            oj.set("quantity", o.quantity); oj.set("destination_node_id", o.destination_node_id); oj.set("radius", o.radius);
            oj.set("required_vehicle_class", o.required_vehicle_class == VehicleClass::Unknown ? "" : vehicleClassName(o.required_vehicle_class));
            oj.set("done", e.state.status == MissionStatus::Completed || static_cast<int>(i) < e.state.current_objective_index);
            oj.set("current", e.state.status == MissionStatus::Active && static_cast<int>(i) == e.state.current_objective_index);
            objs.push(oj);
        }
        m.set("objectives", objs);
        Json rw = Json::object(); rw.set("credits", e.def->rewards.credits); rw.set("local_trust_delta", e.def->rewards.local_trust_delta); rw.set("faction_reputation_delta", e.def->rewards.faction_reputation_delta);
        m.set("rewards", rw);
        arr.push(m);
    }
    return arr;
}

Json JsonApi::stateProjection(int newsCount) const {
    Json r = Json::object();
    const WorldTime& t = world_.time();
    Json time = Json::object(); time.set("tick", static_cast<int64_t>(t.tick)); time.set("day", t.day()); time.set("hour", t.hour()); time.set("minute", t.minute());
    char clock[32]; std::snprintf(clock, sizeof clock, "Day %d %02d:%02d", t.day() + 1, t.hour(), t.minute()); time.set("clock_text", clock);
    r.set("time", time);
    const PlayerState& p = world_.player();
    Json ca = Json::object(); ca.set("act", world_.campaign().act); Json cf = Json::object(); for (const auto& kv : world_.campaign().flags) cf.set(kv.first, kv.second); ca.set("flags", cf); r.set("campaign", ca);
    Json pl = Json::object(); pl.set("entity_id", p.entity_id); pl.set("credits", p.credits); pl.set("region_id", p.region_id); pl.set("pose", p.pose.toJson());
    pl.set("effective_pose", world_.playerEffectivePose().toJson()); pl.set("inventory", mapIntJson(p.inventory)); pl.set("carry_capacity_game_units", p.carry_capacity_game_units);
    r.set("player", pl);
    const PossessionState& po = world_.possession();
    Json pj = Json::object(); pj.set("mode", po.mode == PossessionMode::Seated ? "seated" : "on_foot"); pj.set("vehicle_id", po.vehicle_id); pj.set("seat_id", po.seat_id); pj.set("role", po.role);
    r.set("possession", pj);
    Json vehicles = Json::array();
    for (const auto& kv : world_.vehicles()) {
        const VehicleState& v = kv.second; Json vj = Json::object();
        vj.set("id", kv.first); vj.set("definition_id", v.definition_id); vj.set("region_id", v.region_id); vj.set("pose", v.pose.toJson());
        vj.set("condition_normalized", v.condition_normalized); vj.set("fuel_normalized", v.fuel_normalized); vj.set("altitude_agl", v.altitude_agl); vj.set("cargo", mapIntJson(v.cargo)); vj.set("cargo_units", v.cargoUnits(world_.content()));
        auto d = world_.content().vehicle_definitions.find(v.definition_id);
        vj.set("vehicle_class", d == world_.content().vehicle_definitions.end() ? "unknown" : vehicleClassName(d->second.vehicle_class));
        vj.set("display_name", d == world_.content().vehicle_definitions.end() ? v.definition_id : d->second.display_name);
        Json seats = Json::object(); for (const auto& s : v.occupied_seats) seats.set(s.first, s.second); vj.set("occupied_seats", seats);
        vehicles.push(vj);
    }
    r.set("vehicles", vehicles);
    Json regions = Json::array();
    for (const auto& kv : world_.regions()) {
        Json rj = Json::object(); rj.set("id", kv.first); rj.set("name", kv.second.name); rj.set("control_faction_id", kv.second.control_faction_id);
        rj.set("stability_normalized", kv.second.stability_normalized); rj.set("local_trust", kv.second.local_trust); rj.set("trust_band", reputationBand(kv.second.local_trust));
        const WeatherState* w = world_.regionWeather(kv.first); rj.set("weather", w ? w->condition : "clear"); regions.push(rj);
    }
    r.set("regions", regions);
    Json factions = Json::array();
    for (const auto& kv : world_.factions()) { Json f = Json::object(); f.set("id", kv.first); f.set("name", kv.second.name); f.set("reputation", kv.second.reputation); f.set("band", reputationBand(kv.second.reputation)); factions.push(f); }
    r.set("factions", factions);
    Json nodes = Json::array();
    for (const auto& kv : world_.economyNodes()) {
        Json n = Json::object(); n.set("id", kv.first); n.set("poi_id", kv.second.poi_id); n.set("stocks", mapIntJson(kv.second.stocks)); n.set("capacity_by_item", mapIntJson(kv.second.capacity));
        Json sh = Json::object(); Json prices = Json::object();
        for (const auto& st : kv.second.stocks) { sh.set(st.first, kv.second.shortage(st.first)); auto it = world_.content().items.find(st.first); if (it != world_.content().items.end()) prices.set(st.first, kv.second.priceHint(it->second)); }
        n.set("shortage", sh); n.set("price_hint", prices); nodes.push(n);
    }
    r.set("economy_nodes", nodes);
    Json infra = Json::array();
    for (const auto& kv : world_.infrastructure()) { Json i = Json::object(); i.set("id", kv.first); i.set("status", kv.second.status); auto d = world_.content().infrastructure.find(kv.first); i.set("kind", d == world_.content().infrastructure.end() ? "" : d->second.kind); i.set("poi_id", d == world_.content().infrastructure.end() ? "" : d->second.poi_id); infra.push(i); }
    r.set("infrastructure", infra);
    r.set("missions", missionBoardProjection());
    Json news = Json::array();
    const auto& all = world_.news();
    size_t start = newsCount > 0 && all.size() > static_cast<size_t>(newsCount) ? all.size() - static_cast<size_t>(newsCount) : 0;
    for (size_t i = start; i < all.size(); ++i) { Json n = Json::object(); n.set("news_id", all[i].news_id); n.set("headline", all[i].headline); n.set("tick", static_cast<int64_t>(all[i].tick)); n.set("source_event_id", all[i].source_event_id); news.push(n); }
    r.set("news", news);
    r.set("journal_size", static_cast<int64_t>(world_.journal().size()));
    return r;
}

Json JsonApi::eventsSince(size_t index) const {
    Json arr = Json::array();
    const auto& ev = world_.journal().events();
    for (size_t i = index; i < ev.size(); ++i) arr.push(ev[i].toJson());
    return arr;
}

Json JsonApi::handle(const Json& req) {
    const Json& id = req.get("id");
    const std::string op = req.get("op").asString();
    const Json& a = req.get("args");
    if (op == "load_presets" || op.rfind("realism_", 0) == 0) return handleRealism(id, op, a);
    if (op == "ping") { Json r = Json::object(); r.set("core", "EFWorldCore"); r.set("save_schema", kSaveSchemaVersion); r.set("event_schema", kEventSchemaVersion); r.set("ready", ready_); return ok(id, r); }
    if (op == "start_new_game") {
        // content_text (JSON string) is preferred by hosts whose files live inside a pack (Godot res://); content_path for tools.
        WorldContent c; Status s = a.has("content_text") ? loadContentFromString(a.get("content_text").asString(), c) : loadContentFromFile(a.get("content_path").asString(), c);
        if (!s) return fail(id, s.code, s.message);
        s = world_.initFromContent(c);
        if (!s) return fail(id, s.code, s.message);
        ready_ = true;
        Json r = Json::object(); r.set("content_id", c.content_id); r.set("player_id", world_.player().entity_id); return ok(id, r);
    }
    if (op == "list_saves") {
        Json arr = Json::array(); std::error_code ec;
        if (std::filesystem::exists(saveDir_, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(saveDir_, ec)) {
                const std::string name = entry.path().filename().string();
                if (name.size() <= 7 || name.compare(name.size() - 7, 7, ".efsave") != 0) continue;
                Json j = Json::object(); j.set("slot", name.substr(0, name.size() - 7));
                auto ft = std::filesystem::last_write_time(entry.path(), ec);
                auto sys = std::chrono::time_point_cast<std::chrono::seconds>(ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                j.set("modified_unix", static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(sys.time_since_epoch()).count()));
                std::string bytes; SaveSlotMeta meta; Json snap;
                if (storage_->read(name, bytes).ok && SaveService::parseSaveDocument(bytes, meta, snap).ok) { j.set("world_tick", static_cast<int64_t>(meta.world_tick)); j.set("content_id", meta.content_id); j.set("build_label", meta.build_label); }
                arr.push(j);
            }
        }
        Json r = Json::object(); r.set("saves", arr); return ok(id, r);
    }
    if (!ready_) return fail(id, "not_initialized", "call start_new_game or load first");
    if (op == "content") return ok(id, contentProjection());
    if (op == "state") return ok(id, stateProjection(static_cast<int>(a.get("news_count").asInt(10))));
    if (op == "mission_board") return ok(id, missionBoardProjection());
    if (op == "dialogue") { Json r = Json::object(); r.set("npc_id", a.get("npc_id").asString()); r.set("text", world_.npcDialogue(a.get("npc_id").asString())); return ok(id, r); }
    if (op == "debug_summary") { Json r = Json::object(); r.set("text", world_.debugSummary()); return ok(id, r); }
    if (op == "events_since") return ok(id, eventsSince(static_cast<size_t>(a.get("index").asInt(0))));
    if (op == "apply") {
        Command cmd; Status s = commandFromJson(a.get("command"), cmd);
        if (!s) return fail(id, s.code, s.message);
        size_t before = world_.journal().size();
        CommandResult cr = world_.apply(cmd);
        if (!cr) return fail(id, cr.status.code, cr.status.message);
        Json r = Json::object(); Json ids = Json::array(); for (const auto& e : cr.committed_event_ids) ids.push(e);
        r.set("committed_event_ids", ids); r.set("events", eventsSince(before)); return ok(id, r);
    }
    if (op == "advance_ticks") {
        size_t before = world_.journal().size();
        SimulationReport rep = world_.advanceTicks(a.get("ticks").asInt(0));
        Json r = Json::object(); r.set("ticks_advanced", static_cast<int64_t>(rep.ticks_advanced)); r.set("ticks_remaining", static_cast<int64_t>(rep.ticks_remaining));
        r.set("tier_r_steps", rep.tier_r_steps); r.set("tier_g_steps", rep.tier_g_steps); r.set("tick", static_cast<int64_t>(world_.time().tick));
        r.set("events", a.get("include_events").asBool(false) ? eventsSince(before) : Json::array());
        r.set("new_event_count", static_cast<int64_t>(world_.journal().size() - before));
        return ok(id, r);
    }
    if (op == "save") {
        Status s = saves_->save(world_, a.get("slot").asString(), a.get("build_label").asString().empty() ? "godot-phase1" : a.get("build_label").asString());
        if (!s) return fail(id, s.code, s.message);
        Json r = Json::object(); r.set("slot", a.get("slot").asString()); r.set("path", storage_->pathFor(SaveService::slotKey(a.get("slot").asString()))); return ok(id, r);
    }
    if (op == "load") {
        std::vector<std::string> warnings; SaveSlotMeta meta;
        Status s = saves_->load(world_, a.get("slot").asString(), &warnings, &meta);
        if (!s) return fail(id, s.code, s.message);
        Json r = Json::object(); Json w = Json::array(); for (const auto& x : warnings) w.push(x);
        r.set("warnings", w); r.set("world_tick", static_cast<int64_t>(meta.world_tick)); r.set("content_id", meta.content_id); return ok(id, r);
    }
    if (op == "set_config") {
        SimulationConfig& c = world_.config();
        if (a.has("interact_radius")) c.interact_radius = a.get("interact_radius").asNumber(c.interact_radius);
        if (a.has("board_radius")) c.board_radius = a.get("board_radius").asNumber(c.board_radius);
        if (a.has("delivery_radius")) c.delivery_radius = a.get("delivery_radius").asNumber(c.delivery_radius);
        if (a.has("airborne_exit_height")) c.airborne_exit_height = a.get("airborne_exit_height").asNumber(c.airborne_exit_height);
        return ok(id, Json::object());
    }
    return fail(id, "unknown_op", op);
}

std::string JsonApi::handleText(const std::string& requestJson) {
    Json req; std::string err;
    if (!Json::parse(requestJson, req, &err)) return fail(Json(), "bad_request", err).dump();
    return handle(req).dump();
}

} // namespace ef
