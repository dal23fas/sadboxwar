// Archipelago v2 content + new core features: ReachLocation (class + depth), fuel/refuel/repair,
// mission timers (retry), mission generator (shortage → relief mission, expiry), campaign act switch.
#include "test_helpers.h"
using namespace efh;

static void playHarborLink(WorldState& w) {
    expectOk(moveTo(w, "poi_harbor_west"), "walk to harbor");
    expectOk(w.apply(CmdAcceptMission{"mission_harbor_link"}), "accept harbor link");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_boat_001", "captain"}), "board boat");
    expectOk(moveTo(w, "poi_port_lyria"), "sail to lyria");
    expectOk(w.apply(CmdLoadCargo{"economy_port_lyria", "water_pack", 10}), "load water");
    expectOk(moveTo(w, "poi_pinewatch"), "sail to pinewatch");
    expectOk(w.apply(CmdDeliverCargo{"economy_pinewatch", "water_pack", 10}), "deliver water");
    expectOk(moveTo(w, "poi_harbor_west"), "sail back");
    expectOk(w.apply(CmdExitVehicle{}), "disembark");
    expectOk(w.apply(CmdInteractNpc{"npc_harbor_master"}), "debrief harbor link");
}

EF_TEST(content_v2_loads_two_regions_and_14_missions) {
    WorldState w; EF_REQUIRE(newGame(w));
    EF_CHECK_EQ(w.content().content_id, std::string("asterion_archipelago_v2"));
    EF_CHECK_EQ(w.regions().size(), static_cast<size_t>(2));
    EF_CHECK_EQ(w.missions().size(), static_cast<size_t>(14));
    EF_CHECK_EQ(w.vehicles().size(), static_cast<size_t>(9));
    EF_CHECK_EQ(w.region("region_northshore")->control_faction_id, std::string("faction_northshore"));
    EF_CHECK_EQ(w.infrastructureState("port_cape_varn")->status, std::string("operational"));
}

EF_TEST(reach_location_requires_vehicle_class_and_real_depth) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);
    playHarborLink(w);
    EF_CHECK(w.mission("mission_harbor_link")->status == MissionStatus::Completed);
    expectOk(w.apply(CmdAcceptMission{"mission_reef_survey"}), "accept reef survey");
    // On foot at the reef surface position: not allowed (requires submarine) and too shallow
    Pose reef = poiPose(w, "poi_reef_survey");
    Pose surface = reef; surface.position.z = 0.0;
    expectOk(w.apply(CmdReportPose{"player_main", surface}), "walk?");
    EF_CHECK_EQ(w.mission("mission_reef_survey")->current_objective_index, 1);
    // Boat at the surface above the site: wrong class
    expectOk(moveTo(w, "poi_harbor_west"), "back");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_boat_001", "captain"}), "boat");
    expectOk(w.apply(CmdReportPose{"vehicle_boat_001", surface}), "boat over reef");
    EF_CHECK_EQ(w.mission("mission_reef_survey")->current_objective_index, 1);
    expectOk(moveTo(w, "poi_harbor_west"), "back"); expectOk(w.apply(CmdExitVehicle{}), "exit boat");
    // Submarine at the surface: 40 m above the site -> not yet; dive to -40 -> done
    expectOk(w.apply(CmdEnterVehicle{"vehicle_sub_001", "captain"}), "sub");
    expectOk(w.apply(CmdReportPose{"vehicle_sub_001", surface}), "surface over reef");
    EF_CHECK_EQ(w.mission("mission_reef_survey")->current_objective_index, 1);
    expectOk(w.apply(CmdReportPose{"vehicle_sub_001", reef}), "dive to site");
    EF_CHECK_EQ(w.mission("mission_reef_survey")->current_objective_index, 2);
    expectOk(moveTo(w, "poi_harbor_west"), "surface home");
    Pose home = poiPose(w, "poi_harbor_west"); expectOk(w.apply(CmdReportPose{"vehicle_sub_001", home}), "dock");
    expectOk(w.apply(CmdExitVehicle{}), "exit sub");
    expectOk(w.apply(CmdInteractNpc{"npc_harbor_master"}), "debrief");
    EF_CHECK(w.mission("mission_reef_survey")->status == MissionStatus::Completed);
}

EF_TEST(fuel_burns_with_distance_and_services_cost_credits) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);   // 190 credits; truck now parked in town
    Pose p = w.vehicle("vehicle_truck_001")->pose;
    Pose beside = p; beside.position.x += 2.0; expectOk(w.apply(CmdReportPose{"player_main", beside}), "walk to truck");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"}), "enter");
    double fuel0 = w.vehicle("vehicle_truck_001")->fuel_normalized;
    EF_CHECK(fuel0 < 1.0 && fuel0 > 0.98);   // the delivery run already burned ~2.2 km of a 220 km tank
    // 10 legs of 4 km zig-zagging east/north: ends 20 km away from any facility
    for (int i = 0; i < 10; ++i) { if (i % 2) p.position.y += 4000.0; else p.position.x += 4000.0; expectOk(w.apply(CmdReportPose{"vehicle_truck_001", p}), "drive 4 km"); }
    EF_CHECK_NEAR(w.vehicle("vehicle_truck_001")->fuel_normalized, fuel0 - 40.0 / 220.0, 1e-9);
    EF_CHECK_EQ(w.apply(CmdRefuelVehicle{"vehicle_truck_001"}).status.code, std::string("no_service_here"));   // out in the field
    expectOk(moveTo(w, "poi_depot_south"), "back to depot");   // > 5 km hop: treated as a load/teleport, no burn
    EF_CHECK_NEAR(w.vehicle("vehicle_truck_001")->fuel_normalized, fuel0 - 40.0 / 220.0, 1e-9);
    int credits = w.player().credits;
    expectOk(w.apply(CmdRefuelVehicle{"vehicle_truck_001"}), "refuel");
    EF_CHECK_NEAR(w.vehicle("vehicle_truck_001")->fuel_normalized, 1.0, 1e-9);
    EF_CHECK(w.player().credits < credits);
    EF_CHECK_EQ(w.apply(CmdRefuelVehicle{"vehicle_truck_001"}).status.code, std::string("tank_full"));
    // repair consumes spare parts from the nearby town node and credits
    expectOk(w.apply(CmdSetVehicleCondition{"vehicle_truck_001", 0.5, "test_damage"}), "damage");
    int parts = w.economyNode("economy_port_lyria")->stock("spare_parts");
    credits = w.player().credits;
    expectOk(w.apply(CmdRepairVehicle{"vehicle_truck_001"}), "repair");
    EF_CHECK_NEAR(w.vehicle("vehicle_truck_001")->condition_normalized, 1.0, 1e-9);
    EF_CHECK_EQ(w.economyNode("economy_port_lyria")->stock("spare_parts"), parts - 2);
    EF_CHECK_EQ(w.player().credits, credits - 50);
    EF_CHECK_EQ(w.apply(CmdRepairVehicle{"vehicle_truck_001"}).status.code, std::string("no_damage"));
}

EF_TEST(mission_time_limit_returns_mission_to_available) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);
    expectOk(w.apply(CmdAcceptMission{"mission_mine_supply"}), "accept mine supply (1 day limit)");
    w.advanceTicks(1000);
    EF_CHECK(w.mission("mission_mine_supply")->status == MissionStatus::Active);
    w.advanceTicks(500);
    EF_CHECK(w.mission("mission_mine_supply")->status == MissionStatus::Available);   // retryable: no dead end
    EF_CHECK_EQ(countEvents(w, EventType::MissionFailed), static_cast<size_t>(1));
    expectOk(w.apply(CmdAcceptMission{"mission_mine_supply"}), "accept again");
}

EF_TEST(shortage_generates_relief_mission_that_can_be_completed_or_expires) {
    WorldState w; EF_REQUIRE(newGame(w));
    // Let the clinic run dry: a generated mission must appear (Tier G) and show on the board without an NPC.
    w.advanceTicks(3 * 24 * 60);
    std::string genId;
    for (const auto& kv : w.generatedMissions()) if (kv.second.region_id == "region_asterion_west") { genId = kv.first; break; }
    EF_REQUIRE(!genId.empty());
    const MissionTemplate* def = w.missionDef(genId);
    EF_CHECK(def->generated && def->offeringNpc().empty());
    EF_CHECK(w.mission(genId)->status == MissionStatus::Available);
    bool onBoard = false; for (const auto& e : w.missionBoard()) if (e.def->id == genId) onBoard = true;
    EF_CHECK(onBoard);
    bool newsFound = false; for (const auto& n : w.news()) if (n.headline.find("Relief request") != std::string::npos) newsFound = true;
    EF_CHECK(newsFound);
    // accept anywhere, deliver with the 4x4 from town
    expectOk(w.apply(CmdAcceptMission{genId}), "accept generated");
    const std::string item = def->objectives[0].item_id; const int qty = def->objectives[0].quantity;
    const std::string destNode = def->objectives[0].destination_node_id; const std::string destPoi = w.economyNode(destNode)->poi_id;
    Pose at = w.vehicle("vehicle_4x4_001")->pose; at.position.x += 1; expectOk(w.apply(CmdReportPose{"player_main", at}), "walk to 4x4");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_4x4_001", "driver"}), "enter 4x4");
    expectOk(moveTo(w, "poi_port_lyria"), "to town node");
    // the town may not have enough of the item after a day: top up via the ridge mine? simpler: check stock and load what is needed
    int have = w.economyNode("economy_port_lyria")->stock(item);
    EF_REQUIRE(have >= qty);
    int capacityUnits = 16; int perUnit = w.content().items.at(item).mass_game_units;
    int loadable = std::min(qty, capacityUnits / perUnit);
    int credits = w.player().credits;
    int delivered = 0;
    while (delivered < qty) {
        int n = std::min(loadable, qty - delivered);
        expectOk(moveTo(w, "poi_port_lyria"), "to town");
        expectOk(w.apply(CmdLoadCargo{"economy_port_lyria", item, n}), "load");
        expectOk(moveTo(w, destPoi), "to destination");
        expectOk(w.apply(CmdDeliverCargo{destNode, item, n}), "deliver");
        delivered += n;
    }
    EF_CHECK(w.mission(genId)->status == MissionStatus::Completed);
    EF_CHECK_EQ(w.player().credits, credits + def->rewards.credits);
    // save/load keeps generated missions
    Json snap = w.toSnapshot();
    WorldState l; EF_REQUIRE(newGame(l)); EF_REQUIRE(l.restoreSnapshot(snap).ok);
    EF_CHECK(l.missionDef(genId) != nullptr && l.mission(genId)->status == MissionStatus::Completed);
    EF_CHECK_EQ(l.toSnapshot().dump(), snap.dump());
    // an untouched generated offer expires after 3 days
    WorldState w2; EF_REQUIRE(newGame(w2));
    w2.advanceTicks(3 * 24 * 60);
    std::string g2; for (const auto& kv : w2.generatedMissions()) { g2 = kv.first; break; }
    EF_REQUIRE(!g2.empty());
    w2.advanceTicks(4 * 24 * 60);
    EF_CHECK(w2.mission(g2)->status == MissionStatus::Failed);
    EF_CHECK(w2.generatedMissions().size() <= 12);   // generator never floods the board (max 3 open at a time)
}

EF_TEST(campaign_act_switch_publishes_news) {
    WorldState w; EF_REQUIRE(newGame(w));
    EF_CHECK_EQ(w.campaign().act, std::string("ACT_I_FRACTURED_PEACE"));
    expectOk(w.apply(CmdSetCampaignFlag{"act", "ACT_II_SHIFTING_LINES"}), "act");
    EF_CHECK_EQ(w.campaign().act, std::string("ACT_II_SHIFTING_LINES"));
    bool found = false; for (const auto& n : w.news()) if (n.headline.find("new chapter") != std::string::npos) found = true;
    EF_CHECK(found);
    const MissionTemplate* med = w.missionDef("mission_mediation");
    EF_CHECK(med && med->campaign_flags_on_complete.at("act") == "ACT_II_SHIFTING_LINES");
}

EF_TEST_MAIN()
