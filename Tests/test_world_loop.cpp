// Vertical-slice loop in data form: CORE-01, ROLE-01, MISSION-01, ECON-01, REPU-01, NEWS-01, WS-02, WS-05, WS-07, WS-10.
#include "test_helpers.h"
using namespace efh;

EF_TEST(core01_new_game_creates_world_state) {
    WorldState w; EF_REQUIRE(newGame(w));
    EF_CHECK_EQ(w.player().entity_id, std::string("player_main"));
    EF_CHECK_EQ(w.player().region_id, std::string("region_asterion_west"));
    EF_CHECK_EQ(w.time().tick, static_cast<Tick>(0));
    EF_CHECK_EQ(w.vehicles().size(), static_cast<size_t>(3));
    EF_CHECK_EQ(w.missions().size(), static_cast<size_t>(5));
    EF_CHECK_EQ(w.economyNode("economy_beacon_clinic")->stock("food_crate"), 3);
    EF_CHECK_EQ(w.faction("faction_relief")->reputation, 15);
    EF_CHECK(w.possession().mode == PossessionMode::OnFoot);
    EF_CHECK_EQ(w.journal().size(), static_cast<size_t>(0));
}

EF_TEST(ws10_rejected_commands_do_not_mutate) {
    WorldState w; EF_REQUIRE(newGame(w));
    Json before = w.toSnapshot();
    EF_CHECK_EQ(w.apply(CmdAcceptMission{"mission_road_supplies"}).status.code, std::string("prerequisites_not_met"));
    EF_CHECK_EQ(w.apply(CmdInteractNpc{"npc_harbor_master"}).status.code, std::string("too_far"));
    EF_CHECK_EQ(w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"}).status.code, std::string("too_far"));
    EF_CHECK_EQ(w.apply(CmdExitVehicle{}).status.code, std::string("not_seated"));
    EF_CHECK_EQ(w.apply(CmdLoadCargo{"economy_port_lyria", "food_crate", 0}).status.code, std::string("invalid_quantity"));
    EF_CHECK_EQ(w.apply(CmdLoadCargo{"economy_port_lyria", "food_crate", 5}).status.code, std::string("carrier_capacity_exceeded"));  // 15 units > 12 on foot
    EF_CHECK_EQ(w.apply(CmdDeliverCargo{"economy_port_lyria", "food_crate", 1}).status.code, std::string("insufficient_cargo"));
    EF_CHECK_EQ(w.apply(CmdAcceptMission{"mission_nope"}).status.code, std::string("mission_not_found"));
    EF_CHECK_EQ(w.toSnapshot().dump(), before.dump());
    EF_CHECK_EQ(w.journal().size(), static_cast<size_t>(0));
}

EF_TEST(role01_enter_exit_vehicle_keeps_identity_mission_and_inventory) {
    WorldState w; EF_REQUIRE(newGame(w));
    expectOk(w.apply(CmdAcceptMission{"mission_first_contact"}), "accept");
    expectOk(w.apply(CmdLoadCargo{"economy_port_lyria", "water_pack", 2}), "load on foot");
    EF_CHECK_EQ(w.player().inventory.at("water_pack"), 2);
    expectOk(moveTo(w, "poi_depot_south"), "walk");
    Tick tickBefore = w.time().tick; int credits = w.player().credits;
    expectOk(w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"}), "enter");
    EF_CHECK(w.possession().mode == PossessionMode::Seated);
    EF_CHECK_EQ(w.possession().role, std::string("driver"));
    EF_CHECK_EQ(w.vehicle("vehicle_truck_001")->occupied_seats.at("driver"), std::string("player_main"));
    // WS-02: role change does not touch mission, credits, tick, inventory
    EF_CHECK(w.mission("mission_first_contact")->status == MissionStatus::Active);
    EF_CHECK_EQ(w.player().credits, credits);
    EF_CHECK_EQ(w.time().tick, tickBefore);
    EF_CHECK_EQ(w.player().inventory.at("water_pack"), 2);
    EF_CHECK_EQ(w.apply(CmdEnterVehicle{"vehicle_helo_001", "pilot"}).status.code, std::string("already_seated"));
    expectOk(w.apply(CmdSwitchSeat{"passenger"}), "switch seat");
    EF_CHECK_EQ(w.possession().role, std::string("passenger"));
    EF_CHECK_EQ(w.vehicle("vehicle_truck_001")->occupied_seats.size(), static_cast<size_t>(1));
    expectOk(moveTo(w, "poi_windfield"), "drive");
    expectOk(w.apply(CmdExitVehicle{}), "exit");
    EF_CHECK(w.possession().mode == PossessionMode::OnFoot);
    EF_CHECK_EQ(w.possession().role, std::string("infantry"));
    EF_CHECK(w.vehicle("vehicle_truck_001")->occupied_seats.empty());
    EF_CHECK_NEAR(w.player().pose.position.x, poiPose(w, "poi_windfield").position.x + 3.0, 1e-9);   // exits beside the vehicle
    EF_CHECK_EQ(w.player().inventory.at("water_pack"), 2);
    EF_CHECK_EQ(countEvents(w, EventType::RoleChanged), static_cast<size_t>(3));
}

EF_TEST(mission01_econ01_repu01_news01_full_relief_loop) {
    WorldState w; EF_REQUIRE(newGame(w));
    int clinicFoodBefore = w.economyNode("economy_beacon_clinic")->stock("food_crate");
    int lyriaFoodBefore = w.economyNode("economy_port_lyria")->stock("food_crate");
    int trustBefore = w.region("region_asterion_west")->local_trust;
    int repBefore = w.faction("faction_commonwealth")->reputation;
    int creditsBefore = w.player().credits;
    playThroughRoadSupplies(w);
    EF_CHECK(w.mission("mission_first_contact")->status == MissionStatus::Completed);
    EF_CHECK(w.mission("mission_road_supplies")->status == MissionStatus::Completed);
    // ECON-01: destination stock increases exactly once by delivered amount; source decreased once
    EF_CHECK_EQ(w.economyNode("economy_beacon_clinic")->stock("food_crate"), clinicFoodBefore + 6);
    EF_CHECK_EQ(w.economyNode("economy_port_lyria")->stock("food_crate"), lyriaFoodBefore - 6);
    EF_CHECK(w.vehicle("vehicle_truck_001")->cargo.empty());
    // MISSION-01 / REPU-01: rewards once, as specified in content (30+120 credits, +2+6 trust, +1+2 faction)
    EF_CHECK_EQ(w.player().credits, creditsBefore + 30 + 120);
    EF_CHECK_EQ(w.region("region_asterion_west")->local_trust, trustBefore + 2 + 6);
    EF_CHECK_EQ(w.faction("faction_commonwealth")->reputation, repBefore + 1 + 2);
    EF_CHECK_EQ(countEvents(w, EventType::MissionCompleted), static_cast<size_t>(2));
    EF_CHECK_EQ(countEvents(w, EventType::CivilianReliefDelivered), static_cast<size_t>(1));
    // Repeating the debrief interaction must not pay again
    expectOk(w.apply(CmdInteractNpc{"npc_mayor_elara"}), "talk again");
    EF_CHECK_EQ(w.player().credits, creditsBefore + 150);
    EF_CHECK_EQ(w.apply(CmdAcceptMission{"mission_road_supplies"}).status.code, std::string("mission_not_available"));
    // NEWS-01: news items reference committed events present in the journal
    bool foundRelief = false, foundCompleted = false;
    for (const auto& n : w.news()) {
        EF_CHECK(w.journal().contains(n.source_event_id));
        if (n.headline.find("Food Crate") != std::string::npos) foundRelief = true;
        if (n.headline.find("Road Supplies") != std::string::npos) foundCompleted = true;
    }
    EF_CHECK(foundRelief); EF_CHECK(foundCompleted);
    // WS-07: dialogue reflects authoritative state (trusted line appears once local trust >= 20? here trust=8 -> offer for next mission)
    EF_CHECK(w.npcDialogue("npc_windfield_chief").find("medical packs") != std::string::npos);  // Sky Relief now offered
    EF_CHECK(w.npcDialogue("npc_mayor_elara").find("Not yet") != std::string::npos);            // Community Update locked
}

EF_TEST(ws05_storm_restricts_port_and_mission_modes) {
    WorldState w; EF_REQUIRE(newGame(w));
    playThroughRoadSupplies(w);
    expectOk(moveTo(w, "poi_harbor_west"), "walk to harbor");
    expectOk(w.apply(CmdAcceptMission{"mission_harbor_link"}), "accept harbor link");
    auto allowed = [&](const std::string& mid) {
        for (const auto& e : w.missionBoard()) if (e.def->id == mid) return e.allowed_vehicle_classes;
        return std::vector<VehicleClass>{};
    };
    EF_CHECK_EQ(allowed("mission_harbor_link").size(), static_cast<size_t>(2));   // naval + rotorcraft
    expectOk(w.apply(CmdSetWeather{"region_asterion_west", "storm"}), "storm");
    EF_CHECK_EQ(w.infrastructureState("port_harbor_west")->status, std::string("restricted"));
    EF_CHECK_EQ(w.infrastructureState("airfield_windfield")->status, std::string("restricted"));
    EF_CHECK_EQ(allowed("mission_harbor_link").size(), static_cast<size_t>(0));
    EF_CHECK_EQ(w.apply(CmdEnterVehicle{"vehicle_boat_001", "captain"}).status.code, std::string("port_restricted"));
    expectOk(w.apply(CmdSetWeather{"region_asterion_west", "clear"}), "clear");
    EF_CHECK_EQ(w.infrastructureState("port_harbor_west")->status, std::string("operational"));
    expectOk(w.apply(CmdEnterVehicle{"vehicle_boat_001", "captain"}), "board boat");
    EF_CHECK_EQ(w.possession().role, std::string("captain"));
    // Harbor Link by sea: load water at Lyria (boat must be near the town node), sail to Pinewatch, deliver 10
    expectOk(moveTo(w, "poi_port_lyria"), "sail to lyria quay");
    expectOk(w.apply(CmdLoadCargo{"economy_port_lyria", "water_pack", 10}), "load water");
    expectOk(moveTo(w, "poi_pinewatch"), "sail to pinewatch");
    int before = w.economyNode("economy_pinewatch")->stock("water_pack");
    expectOk(w.apply(CmdDeliverCargo{"economy_pinewatch", "water_pack", 10}), "deliver water");
    EF_CHECK_EQ(w.economyNode("economy_pinewatch")->stock("water_pack"), before + 10);
    EF_CHECK(w.mission("mission_harbor_link")->current_objective_index == 2);   // debrief pending
    expectOk(moveTo(w, "poi_harbor_west"), "sail back");
    expectOk(w.apply(CmdExitVehicle{}), "disembark");
    expectOk(w.apply(CmdInteractNpc{"npc_harbor_master"}), "debrief");
    EF_CHECK(w.mission("mission_harbor_link")->status == MissionStatus::Completed);
    // Same economy was affected by two distinct vehicle roles (truck + boat) in one world state (Phase 5 acceptance)
    EF_CHECK_EQ(countEvents(w, EventType::CivilianReliefDelivered), static_cast<size_t>(2));
}

EF_TEST(helicopter_cannot_be_exited_airborne) {
    WorldState w; EF_REQUIRE(newGame(w));
    expectOk(moveTo(w, "poi_windfield"), "walk");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_helo_001", "pilot"}), "board helo");
    Pose air = poiPose(w, "poi_beacon_clinic"); air.position.z = 120.0;
    expectOk(w.apply(CmdReportPose{"vehicle_helo_001", air}), "fly");
    EF_CHECK_EQ(w.apply(CmdExitVehicle{}).status.code, std::string("exit_unsafe_airborne"));
    air.position.z = 0.0;
    expectOk(w.apply(CmdReportPose{"vehicle_helo_001", air}), "land");
    expectOk(w.apply(CmdExitVehicle{}), "exit on ground");
    EF_CHECK(w.possession().mode == PossessionMode::OnFoot);
}

EF_TEST_MAIN()
