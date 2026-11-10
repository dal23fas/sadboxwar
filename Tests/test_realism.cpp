// Supplement tests: T10 (global preset + category override together), T16 (reset returns exactly the preset),
// T11 settings half (settings persist separately from world saves), and the architectural invariant of §14:
// realism settings never change world-state outcomes.
#include "test_helpers.h"
#include "EFWorldCore/RealismProfiles.h"
#include "EFWorldCore/JsonApi.h"
#include <filesystem>
using namespace efh;

static std::string presetsPath() { std::string p = EF_CONTENT_PATH; return p.substr(0, p.rfind('/')) + "/realism_presets_v1.json"; }

EF_TEST(presets_load_and_validate) {
    RealismProfileManager m;
    EF_REQUIRE(m.loadPresetsFromFile(presetsPath()).ok);
    EF_CHECK_EQ(m.categories().size(), static_cast<size_t>(8));
    EF_CHECK_EQ(m.globalPreset(), std::string("BALANCED"));
    ResolvedProfile h = m.resolve("helicopter");
    EF_CHECK_EQ(h.preset_label, std::string("BALANCED"));
    EF_CHECK_EQ(h.values.at("auto_stabilization"), 60);
    // malformed presets rejected: out-of-range value
    Json bad = Json::object(); bad.set("preset_names", Json::array().push("A")); Json cats = Json::object(); Json cat = Json::object(); Json A = Json::object(); A.set("x", 140); cat.set("A", A); cats.set("c", cat); bad.set("categories", cats);
    RealismProfileManager m2; EF_CHECK(!m2.loadPresets(bad).ok);
}

EF_TEST(t10_global_preset_and_category_override_work_together) {
    RealismProfileManager m; EF_REQUIRE(m.loadPresetsFromFile(presetsPath()).ok);
    EF_CHECK(m.setGlobalPreset("ARCADE").ok);
    EF_CHECK(m.setCategoryPreset("helicopter", "REALISTIC").ok);
    EF_CHECK(m.setCategoryPreset("submarine", "BALANCED").ok);
    EF_CHECK(m.setOverride("infantry", "fatigue_rate", 65).ok);
    EF_CHECK_EQ(m.resolve("tracked").preset_label, std::string("ARCADE"));       // follows global
    EF_CHECK_EQ(m.resolve("tracked").values.at("inertia_influence"), 35);
    EF_CHECK_EQ(m.resolve("helicopter").preset_label, std::string("REALISTIC")); // category override
    EF_CHECK_EQ(m.resolve("helicopter").values.at("inertia_influence"), 85);
    EF_CHECK_EQ(m.resolve("submarine").values.at("depth_hold_assist"), 60);
    ResolvedProfile inf = m.resolve("infantry");
    EF_CHECK_EQ(inf.preset_label, std::string("CUSTOM"));                        // slider makes it CUSTOM
    EF_CHECK_EQ(inf.base_preset, std::string("ARCADE"));
    EF_CHECK_EQ(inf.values.at("fatigue_rate"), 65);
    EF_CHECK_EQ(inf.values.at("aim_assist"), 70);                                 // other values still from ARCADE
    EF_CHECK_EQ(inf.overridden.size(), static_cast<size_t>(1));
    // invalid input leaves settings unchanged
    EF_CHECK(!m.setOverride("infantry", "warp_drive", 10).ok);
    EF_CHECK(!m.setOverride("infantry", "fatigue_rate", 101).ok);
    EF_CHECK(!m.setCategoryPreset("spaceship", "ARCADE").ok);
    EF_CHECK(!m.setGlobalPreset("ULTRA").ok);
    EF_CHECK_EQ(m.resolve("infantry").values.at("fatigue_rate"), 65);
    // combat challenge is independent from physics realism (§11)
    EF_CHECK(m.setCombatChallenge("HARD").ok);
    EF_CHECK_EQ(m.resolve("tracked").preset_label, std::string("ARCADE"));
}

EF_TEST(t16_reset_restores_exact_preset_and_settings_roundtrip) {
    RealismProfileManager m; EF_REQUIRE(m.loadPresetsFromFile(presetsPath()).ok);
    Json pristineBalanced = m.fullProjection().get("resolved");
    EF_CHECK(m.setGlobalPreset("REALISTIC").ok);
    Json pristineRealistic = m.fullProjection().get("resolved");
    EF_CHECK(m.setOverride("wheeled", "steering_assist", 99).ok);
    EF_CHECK(m.setOverride("wheeled", "inertia_influence", 1).ok);
    EF_CHECK(m.setCategoryPreset("fixed_wing", "ARCADE").ok);
    EF_CHECK_EQ(m.resolve("wheeled").preset_label, std::string("CUSTOM"));
    // per-category reset -> exactly the global preset values for that category
    EF_CHECK(m.resetCategory("wheeled").ok);
    EF_CHECK_EQ(m.resolve("wheeled").toJson().dump(), pristineRealistic.get("wheeled").dump());
    EF_CHECK_EQ(m.resolve("fixed_wing").preset_label, std::string("ARCADE"));  // untouched by the other reset
    // settings persist and round-trip exactly (T11, settings half)
    Json saved = m.settingsToJson();
    RealismProfileManager m2; EF_REQUIRE(m2.loadPresetsFromFile(presetsPath()).ok);
    EF_REQUIRE(m2.settingsFromJson(saved).ok);
    EF_CHECK_EQ(m2.fullProjection().dump(), m.fullProjection().dump());
    // corrupt settings are rejected without touching current settings
    Json corrupt = saved; Json ov = Json::object(); Json w = Json::object(); w.set("steering_assist", 500); ov.set("wheeled", w); corrupt.set("custom_overrides", ov);
    EF_CHECK(!m2.settingsFromJson(corrupt).ok);
    EF_CHECK_EQ(m2.fullProjection().dump(), m.fullProjection().dump());
    // full reset -> exactly the default global preset
    m.resetAll();
    EF_CHECK_EQ(m.fullProjection().get("resolved").dump(), pristineBalanced.dump());
}

// §14 invariant: the same mission produces the same world events whatever the realism settings.
// The core has no code path reading realism settings; this test pins that down through the JSON API.
EF_TEST(realism_settings_do_not_change_world_outcomes) {
    auto run = [&](const char* preset, const char* dir) {
        std::string d = std::string(EF_TEST_TMP) + "/" + dir; std::error_code ec; std::filesystem::remove_all(d, ec);
        JsonApi api(d);
        auto call = [&](const char* op, Json args) { Json r = Json::object(); r.set("id", 1); r.set("op", op); r.set("args", args); return api.handle(r); };
        Json pp = Json::object(); pp.set("path", presetsPath()); EF_CHECK(call("load_presets", pp).get("ok").asBool());
        Json gp = Json::object(); gp.set("preset", preset); EF_CHECK(call("realism_set_global", gp).get("ok").asBool());
        Json cp = Json::object(); cp.set("content_path", EF_CONTENT_PATH); EF_CHECK(call("start_new_game", cp).get("ok").asBool());
        playThroughRoadSupplies(api.world());
        api.world().advanceTicks(300);
        Json s = api.world().toSnapshot(); return s.dump();
    };
    std::string arcade = run("ARCADE", "rl_arcade");
    std::string realistic = run("REALISTIC", "rl_realistic");
    EF_CHECK_EQ(arcade, realistic);
}

EF_TEST(submarine_is_a_distinct_class_bound_to_port_rules) {
    WorldState w; EF_REQUIRE(newGame(w));
    const VehicleState* sub = w.vehicle("vehicle_sub_001"); EF_REQUIRE(sub != nullptr);
    const VehicleDefinition& def = w.content().vehicle_definitions.at(sub->definition_id);
    EF_CHECK(def.vehicle_class == VehicleClass::Submarine);
    EF_CHECK_EQ(def.realism_category, std::string("submarine"));
    EF_CHECK_EQ(def.handling.get("max_depth_m").asInt(), static_cast<int64_t>(120));
    expectOk(w.apply(CmdSetWeather{"region_asterion_west", "storm"}), "storm");
    Pose p = sub->pose; p.position.x -= 1; expectOk(w.apply(CmdReportPose{"player_main", p}), "walk to sub");
    EF_CHECK_EQ(w.apply(CmdEnterVehicle{"vehicle_sub_001", "captain"}).status.code, std::string("port_restricted"));
    expectOk(w.apply(CmdSetWeather{"region_asterion_west", "clear"}), "clear");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_sub_001", "captain"}), "board sub");
    EF_CHECK_EQ(w.possession().role, std::string("captain"));
    bool subAllowed = false;
    for (const auto& e : w.missionBoard()) if (e.def->id == "mission_harbor_link") for (VehicleClass c : e.allowed_vehicle_classes) if (c == VehicleClass::Submarine) subAllowed = true;
    EF_CHECK(subAllowed);
}

EF_TEST_MAIN()
