// WORLD-01, WORLD-02, WS-06: offscreen aggregate simulation, determinism, bounded catch-up.
#include "test_helpers.h"
using namespace efh;

EF_TEST(world01_offscreen_ticks_advance_economy_without_presentation) {
    WorldState w; EF_REQUIRE(newGame(w));
    int clinicWater = w.economyNode("economy_beacon_clinic")->stock("water_pack");
    int lyriaWater = w.economyNode("economy_port_lyria")->stock("water_pack");
    SimulationReport rep = w.advanceTicks(540);   // 3 Tier-R steps (every 3 h), 9 Tier-G steps
    EF_CHECK_EQ(rep.ticks_advanced, static_cast<Tick>(540));
    EF_CHECK_EQ(rep.tier_r_steps, 3);
    EF_CHECK_EQ(rep.tier_g_steps, 9);
    EF_CHECK_EQ(w.economyNode("economy_beacon_clinic")->stock("water_pack"), clinicWater - 3);  // consumption 1/step
    EF_CHECK_EQ(w.economyNode("economy_port_lyria")->stock("water_pack"), lyriaWater + 3);      // +2 production -1 consumption
    EF_CHECK(countEvents(w, EventType::ResourceStockChanged) > 0);
    EF_CHECK_EQ(w.time().hour(), 17); EF_CHECK_EQ(w.time().minute(), 0);
    // stocks never go negative, stop at zero
    w.advanceTicks(3000);
    for (const auto& kv : w.economyNodes()) for (const auto& st : kv.second.stocks) EF_CHECK(st.second >= 0);
    EF_CHECK_EQ(w.economyNode("economy_beacon_clinic")->stock("medical_pack"), 0);
    // shortage news was generated from a system event and references it
    bool shortageNews = false;
    for (const auto& n : w.news()) if (n.headline.find("Shortage") != std::string::npos) { shortageNews = true; EF_CHECK(w.journal().contains(n.source_event_id)); }
    EF_CHECK(shortageNews);
    // Civic stability dropped under sustained shortage (Tier G)
    EF_CHECK(w.region("region_asterion_west")->stability_normalized < 0.78);
}

EF_TEST(ws06_simulation_is_deterministic_for_same_seed_and_save_load) {
    WorldState a; EF_REQUIRE(newGame(a));
    WorldState b; EF_REQUIRE(newGame(b));
    a.advanceTicks(24 * 60 * 3);
    b.advanceTicks(24 * 60);
    Json mid = b.toSnapshot();
    WorldState c; EF_REQUIRE(newGame(c)); EF_REQUIRE(c.restoreSnapshot(mid).ok);  // "load" after one day
    c.advanceTicks(24 * 60 * 2);
    EF_CHECK_EQ(a.toSnapshot().dump(), c.toSnapshot().dump());
    EF_CHECK(countEvents(a, EventType::WeatherChanged) > 0);   // weather actually varies over 3 days with this seed
    // different seed -> different weather history (sanity that the seed matters)
    WorldContent other; EF_REQUIRE(loadContentFromFile(EF_CONTENT_PATH, other).ok); other.simulation_seed = 999;
    WorldState d; EF_REQUIRE(d.initFromContent(other).ok); d.advanceTicks(24 * 60 * 3);
    EF_CHECK(countEvents(d, EventType::WeatherChanged) != countEvents(a, EventType::WeatherChanged) || d.toSnapshot().get("weather").dump() != a.toSnapshot().get("weather").dump());
}

EF_TEST(catch_up_is_bounded_per_call) {
    WorldState w; EF_REQUIRE(newGame(w));
    w.config().max_catchup_ticks_per_call = 100;
    SimulationReport rep = w.advanceTicks(250);
    EF_CHECK_EQ(rep.ticks_advanced, static_cast<Tick>(100));
    EF_CHECK_EQ(rep.ticks_remaining, static_cast<Tick>(150));
    EF_CHECK_EQ(w.time().tick, static_cast<Tick>(100));
}

EF_TEST(world02_weather_from_simulation_restricts_harbor_then_recovers) {
    WorldState w; EF_REQUIRE(newGame(w));
    // Drive the simulation until a storm occurs (seeded; must happen within a few days for this content seed)
    bool sawStorm = false, sawRestricted = false, sawReopened = false;
    for (int day = 0; day < 10 && !sawReopened; ++day) {
        w.advanceTicks(60);
        const std::string cond = w.regionWeather("region_asterion_west")->condition;
        const std::string port = w.infrastructureState("port_harbor_west")->status;
        if (cond == "storm") { sawStorm = true; EF_CHECK_EQ(port, std::string("restricted")); sawRestricted = true; }
        if (sawRestricted && cond != "storm") { EF_CHECK_EQ(port, std::string("operational")); sawReopened = true; }
    }
    if (!sawStorm) { for (int h = 0; h < 24 * 10 && !sawStorm; ++h) { w.advanceTicks(60); sawStorm = w.regionWeather("region_asterion_west")->condition == "storm"; } }
    EF_CHECK(sawStorm);
    // Infrastructure status and weather are always consistent after any number of ticks
    const bool storm = w.regionWeather("region_asterion_west")->condition == "storm";
    EF_CHECK_EQ(w.infrastructureState("port_harbor_west")->status, std::string(storm ? "restricted" : "operational"));
}

EF_TEST_MAIN()
