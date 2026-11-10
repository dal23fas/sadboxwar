// SAVE-01, SAVE-02, WS-04, WS-09: persistence, seated restore/fallback, no duplicate rewards, journal replay == snapshot, rollback.
#include "test_helpers.h"
#include "EFWorldCore/SaveService.h"
#include <filesystem>
#include <fstream>
using namespace efh;
namespace fs = std::filesystem;

static std::string tmpDir(const char* name) { std::string d = std::string(EF_TEST_TMP) + "/" + name; std::error_code ec; fs::remove_all(d, ec); fs::create_directories(d, ec); return d; }

EF_TEST(save01_on_foot_save_and_reload_restores_everything) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);
    w.advanceTicks(95);
    expectOk(w.apply(CmdSetCampaignFlag{"met_mayor", "true"}), "flag");
    FileSaveStorage storage(tmpDir("save01")); SaveService svc(storage);
    EF_REQUIRE(svc.save(w, "slot1").ok);
    EF_CHECK(storage.exists("slot1.efsave"));
    WorldState loaded; EF_REQUIRE(newGame(loaded));
    std::vector<std::string> warnings; SaveSlotMeta meta;
    Status s = svc.load(loaded, "slot1", &warnings, &meta);
    if (!s) std::printf("load: %s %s\n", s.code.c_str(), s.message.c_str());
    EF_REQUIRE(s.ok);
    EF_CHECK(warnings.empty());
    EF_CHECK_EQ(meta.world_tick, static_cast<Tick>(95));
    EF_CHECK_EQ(loaded.toSnapshot().dump(), w.toSnapshot().dump());   // byte-identical authoritative state
    EF_CHECK_EQ(loaded.time().tick, static_cast<Tick>(95));
    EF_CHECK_EQ(loaded.player().credits, w.player().credits);
    EF_CHECK_EQ(loaded.campaign().flags.at("met_mayor"), std::string("true"));
    EF_CHECK_EQ(loaded.journal().size(), w.journal().size());
    EF_CHECK_EQ(loaded.news().size(), w.news().size());
    // Second save rotates the previous file into .bak
    EF_REQUIRE(svc.save(loaded, "slot1").ok);
    EF_CHECK(storage.exists("slot1.efsave.bak"));
}

EF_TEST(save02_seated_save_restores_seat_or_falls_back) {
    WorldState w; EF_REQUIRE(newGame(w));
    expectOk(moveTo(w, "poi_depot_south"), "walk");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"}), "enter");
    expectOk(moveTo(w, "poi_windfield"), "drive");
    FileSaveStorage storage(tmpDir("save02")); SaveService svc(storage);
    EF_REQUIRE(svc.save(w, "seated").ok);
    WorldState a; EF_REQUIRE(newGame(a));
    std::vector<std::string> warnings;
    EF_REQUIRE(svc.load(a, "seated", &warnings).ok);
    EF_CHECK(warnings.empty());
    EF_CHECK(a.possession().mode == PossessionMode::Seated);
    EF_CHECK_EQ(a.possession().vehicle_id, std::string("vehicle_truck_001"));
    EF_CHECK_EQ(a.possession().seat_id, std::string("driver"));
    EF_CHECK_NEAR(a.vehicle("vehicle_truck_001")->pose.position.x, poiPose(a, "poi_windfield").position.x, 1e-9);
    expectOk(a.apply(CmdExitVehicle{}), "exit after load");

    // Fallback: save references a vehicle that no longer exists in content -> on-foot at spawn, with warning (WS-09)
    Json snap = w.toSnapshot();
    Json possession = snap.get("possession"); possession.set("vehicle_id", "vehicle_missing_999"); snap.set("possession", possession);
    WorldState b; EF_REQUIRE(newGame(b));
    warnings.clear();
    EF_REQUIRE(b.restoreSnapshot(snap, &warnings).ok);
    EF_CHECK(!warnings.empty());
    EF_CHECK(b.possession().mode == PossessionMode::OnFoot);
    EF_CHECK_NEAR(b.player().pose.position.x, poiPose(b, "poi_port_lyria").position.x, 1e-9);
    EF_CHECK(b.vehicle("vehicle_truck_001")->occupied_seats.empty());   // stale seat occupancy cleared
}

EF_TEST(ws04_deliver_save_load_no_duplicate_reward_and_replay_matches) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);
    int credits = w.player().credits; int clinicFood = w.economyNode("economy_beacon_clinic")->stock("food_crate");
    FileSaveStorage storage(tmpDir("ws04")); SaveService svc(storage);
    EF_REQUIRE(svc.save(w, "s").ok);
    WorldState l; EF_REQUIRE(newGame(l)); EF_REQUIRE(svc.load(l, "s").ok);
    EF_CHECK_EQ(l.player().credits, credits);
    EF_CHECK_EQ(l.economyNode("economy_beacon_clinic")->stock("food_crate"), clinicFood);
    // Re-ingesting every journal event into the loaded world is a no-op (idempotent replay)
    for (const auto& e : w.journal().events()) { bool applied = true; EF_CHECK(l.ingestEvent(e, &applied).ok); EF_CHECK(!applied); }
    EF_CHECK_EQ(l.player().credits, credits);
    EF_CHECK_EQ(l.economyNode("economy_beacon_clinic")->stock("food_crate"), clinicFood);
    EF_CHECK_EQ(countEvents(l, EventType::MissionCompleted), static_cast<size_t>(2));
    // Replaying the journal over fresh content reproduces the snapshot (reducers are the only mutation path)
    WorldState r; EF_REQUIRE(newGame(r));
    for (const auto& e : w.journal().events()) EF_CHECK(r.ingestEvent(e).ok);
    Json rs = r.toSnapshot(), ws = w.toSnapshot();
    // The replayed world never generated primary IDs itself, so only the sequence counter may differ.
    Json rj = rs.get("journal"); rj.set("sequence", 0); rs.set("journal", rj);
    Json wj = ws.get("journal"); wj.set("sequence", 0); ws.set("journal", wj);
    EF_CHECK_EQ(rs.dump(), ws.dump());
    EF_CHECK_EQ(r.journal().size(), w.journal().size());
}

EF_TEST(corrupt_primary_rolls_back_to_backup) {
    WorldState w; EF_REQUIRE(newGame(w));
    FileSaveStorage storage(tmpDir("rollback")); SaveService svc(storage);
    EF_REQUIRE(svc.save(w, "x").ok);
    w.advanceTicks(30);
    EF_REQUIRE(svc.save(w, "x").ok);   // now x.efsave (tick 30) + x.efsave.bak (tick 0)
    { std::ofstream f(storage.pathFor("x.efsave"), std::ios::trunc); f << "{ corrupted"; }
    WorldState l; EF_REQUIRE(newGame(l));
    std::vector<std::string> warnings;
    EF_REQUIRE(svc.load(l, "x", &warnings).ok);
    EF_CHECK(!warnings.empty());
    EF_CHECK_EQ(l.time().tick, static_cast<Tick>(0));
    // unsupported schema is refused cleanly
    Json snap = w.toSnapshot(); snap.set("schema_version", 99);
    EF_CHECK_EQ(l.restoreSnapshot(snap).code, std::string("save_schema_unsupported"));
}

EF_TEST_MAIN()
