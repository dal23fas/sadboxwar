// JsonApi: the contract Godot (GDExtension or TCP) depends on. Runs the slice through the JSON surface only.
#include "ef_test.h"
#include "EFWorldCore/JsonApi.h"
#include <filesystem>
using namespace ef;

static Json call(JsonApi& api, const char* op, Json args = Json::object(), int id = 1) {
    Json req = Json::object(); req.set("id", id); req.set("op", op); req.set("args", args);
    return api.handle(req);
}
static Json cmd(const char* type) { Json c = Json::object(); c.set("type", type); return c; }
static Json poseAt(double x, double y, double z = 0) { Pose p; p.position = {x, y, z}; return p.toJson(); }

EF_TEST(json_api_runs_first_contact_and_truck_loop) {
    std::string dir = std::string(EF_TEST_TMP) + "/json_api"; std::error_code ec; std::filesystem::remove_all(dir, ec);
    JsonApi api(dir);
    Json r = call(api, "state");
    EF_CHECK(!r.get("ok").asBool(true)); EF_CHECK_EQ(r.get("code").asString(), std::string("not_initialized"));
    Json a = Json::object(); a.set("content_path", EF_CONTENT_PATH);
    r = call(api, "start_new_game", a, 7);
    EF_REQUIRE(r.get("ok").asBool()); EF_CHECK_EQ(r.get("id").asInt(), static_cast<int64_t>(7));
    EF_CHECK_EQ(r.get("result").get("content_id").asString(), std::string("asterion_archipelago_v2"));

    r = call(api, "content");
    EF_CHECK_EQ(r.get("result").get("pois").size(), static_cast<size_t>(11));
    EF_CHECK_EQ(r.get("result").get("npcs").size(), static_cast<size_t>(9));

    Json c = cmd("AcceptMission"); c.set("mission_id", "mission_first_contact");
    Json args = Json::object(); args.set("command", c);
    r = call(api, "apply", args);
    EF_REQUIRE(r.get("ok").asBool());
    EF_CHECK_EQ(r.get("result").get("committed_event_ids").size(), static_cast<size_t>(2));
    EF_CHECK_EQ(r.get("result").get("events").asArray()[0].get("event_type").asString(), std::string("MissionAccepted"));

    // rejected command -> ok:false with code, nothing committed
    c = cmd("InteractNpc"); c.set("npc_id", "npc_harbor_master"); args.set("command", c);
    r = call(api, "apply", args);
    EF_CHECK(!r.get("ok").asBool(true)); EF_CHECK_EQ(r.get("code").asString(), std::string("too_far"));

    // move (as the presentation would), enter truck, drive, state projection reflects it
    c = cmd("ReportPose"); c.set("entity_id", "player_main"); c.set("pose", poseAt(100, -250)); args.set("command", c);
    EF_CHECK(call(api, "apply", args).get("ok").asBool());
    c = cmd("EnterVehicle"); c.set("vehicle_id", "vehicle_truck_001"); c.set("seat_id", "driver"); args.set("command", c);
    EF_CHECK(call(api, "apply", args).get("ok").asBool());
    r = call(api, "state");
    EF_CHECK_EQ(r.get("result").get("possession").get("role").asString(), std::string("driver"));
    EF_CHECK_EQ(r.get("result").get("time").get("clock_text").asString(), std::string("Day 1 08:00"));
    bool truckFound = false;
    for (const auto& v : r.get("result").get("vehicles").asArray()) if (v.get("id").asString() == "vehicle_truck_001") { truckFound = true; EF_CHECK_EQ(v.get("occupied_seats").get("driver").asString(), std::string("player_main")); EF_CHECK_EQ(v.get("vehicle_class").asString(), std::string("ground")); }
    EF_CHECK(truckFound);
    bool firstContactActive = false;
    for (const auto& m : r.get("result").get("missions").asArray()) if (m.get("mission_id").asString() == "mission_first_contact") { firstContactActive = m.get("status").asString() == "Active"; EF_CHECK(m.get("objectives").asArray()[1].get("current").asBool()); }
    EF_CHECK(firstContactActive);

    Json t = Json::object(); t.set("ticks", 125);
    r = call(api, "advance_ticks", t);
    EF_CHECK_EQ(r.get("result").get("tick").asInt(), static_cast<int64_t>(125));
    EF_CHECK_EQ(r.get("result").get("tier_g_steps").asInt(), static_cast<int64_t>(2));
    EF_CHECK_EQ(call(api, "state").get("result").get("time").get("clock_text").asString(), std::string("Day 1 10:05"));

    Json d = Json::object(); d.set("npc_id", "npc_mayor_elara");
    EF_CHECK(call(api, "dialogue", d).get("result").get("text").asString().find("isn't finished") != std::string::npos);

    Json s = Json::object(); s.set("slot", "api_slot");
    r = call(api, "save", s); EF_REQUIRE(r.get("ok").asBool());
    Json before = api.world().toSnapshot();
    JsonApi api2(dir);
    EF_REQUIRE(call(api2, "start_new_game", a).get("ok").asBool());
    r = call(api2, "load", s); EF_REQUIRE(r.get("ok").asBool());
    EF_CHECK_EQ(r.get("result").get("world_tick").asInt(), static_cast<int64_t>(125));
    EF_CHECK_EQ(api2.world().toSnapshot().dump(), before.dump());

    // events_since lets presentation catch up on everything it missed
    r = call(api2, "events_since", Json::object());
    EF_CHECK_EQ(r.get("result").size(), api2.world().journal().size());
    EF_CHECK_EQ(call(api2, "nope").get("code").asString(), std::string("unknown_op"));
    EF_CHECK(!Json().isObject());
    std::string txt = api2.handleText("{not json");
    Json parsed; EF_CHECK(Json::parse(txt, parsed)); EF_CHECK_EQ(parsed.get("code").asString(), std::string("bad_request"));
}

EF_TEST_MAIN()
