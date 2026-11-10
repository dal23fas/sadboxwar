// ef_headless — runs the Asterion vertical-slice acceptance script (Roadmap §10) against the
// authoritative core with no 3D presentation. Movement is reported as poses (what the engine's
// presentation layer will do). Prints the committed journal and the world debug summary.
//
//   ef_headless <content.json> [save_dir]
//
// Exit code 0 only if every step produced the expected persistent outcome.
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/ContentLoader.h"
#include "EFWorldCore/SaveService.h"
#include <cstdio>
#include <string>

using namespace ef;

static int g_failures = 0;
static void step(int n, const char* action, bool ok, const std::string& detail) {
    std::printf("[%2d] %-48s %s %s\n", n, action, ok ? "OK  " : "FAIL", detail.c_str());
    if (!ok) ++g_failures;
}
static std::string why(const CommandResult& r) { return r ? std::to_string(r.committed_event_ids.size()) + " event(s)" : r.status.code + ": " + r.status.message; }
static Pose poiPose(const WorldState& w, const std::string& id) { auto it = w.content().pois.find(id); return it == w.content().pois.end() ? Pose{} : it->second.pose; }
static CommandResult moveTo(WorldState& w, const std::string& poiId) {
    Pose p = poiPose(w, poiId);
    if (w.possession().mode == PossessionMode::Seated) return w.apply(CmdReportPose{w.possession().vehicle_id, p});
    return w.apply(CmdReportPose{w.player().entity_id, p});
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: ef_headless <content.json> [save_dir]\n"); return 2; }
    const std::string saveDir = argc > 2 ? argv[2] : "./Saved";
    WorldContent content;
    Status s = loadContentFromFile(argv[1], content);
    if (!s) { std::fprintf(stderr, "content: %s %s\n", s.code.c_str(), s.message.c_str()); return 2; }
    WorldState w;
    s = w.initFromContent(content);
    step(1, "Start new game in Port Lyria", s.ok && w.player().entity_id == "player_main" && w.time().tick == 0, s.ok ? "player_main @ tick 0" : s.message);

    CommandResult r = w.apply(CmdAcceptMission{"mission_first_contact"});
    step(2, "Talk to Mayor Elara Venn (accept First Contact)", r && w.mission("mission_first_contact")->status == MissionStatus::Active, why(r));
    std::printf("     mayor: \"%s\"\n", w.npcDialogue("npc_mayor_elara").c_str());
    moveTo(w, "poi_harbor_west"); w.apply(CmdInteractNpc{"npc_harbor_master"}); moveTo(w, "poi_port_lyria");
    r = w.apply(CmdInteractNpc{"npc_mayor_elara"});
    step(2, "Debrief First Contact", r && w.mission("mission_first_contact")->status == MissionStatus::Completed, why(r));
    r = w.apply(CmdAcceptMission{"mission_road_supplies"});
    step(2, "Accept Road Supplies", r.status.ok, why(r));

    moveTo(w, "poi_depot_south");
    r = w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"});
    step(3, "Enter and drive truck", r && w.possession().role == "driver" && w.mission("mission_road_supplies")->status == MissionStatus::Active, why(r));
    moveTo(w, "poi_port_lyria");
    r = w.apply(CmdLoadCargo{"economy_port_lyria", "food_crate", 6});
    step(3, "Load 6 food crates at Port Lyria", r.status.ok, why(r));
    moveTo(w, "poi_beacon_clinic");
    int before = w.economyNode("economy_beacon_clinic")->stock("food_crate");
    r = w.apply(CmdDeliverCargo{"economy_beacon_clinic", "food_crate", 6});
    step(4, "Deliver goods (stock changes exactly once)", r && w.economyNode("economy_beacon_clinic")->stock("food_crate") == before + 6, why(r) + ", clinic food " + std::to_string(before) + " -> " + std::to_string(w.economyNode("economy_beacon_clinic")->stock("food_crate")));
    moveTo(w, "poi_port_lyria"); w.apply(CmdExitVehicle{});
    int credits = w.player().credits;
    r = w.apply(CmdInteractNpc{"npc_mayor_elara"});
    step(4, "Debrief Road Supplies (reward once)", r && w.mission("mission_road_supplies")->status == MissionStatus::Completed && w.player().credits == credits + 120, why(r) + ", credits " + std::to_string(credits) + " -> " + std::to_string(w.player().credits));

    moveTo(w, "poi_windfield");
    r = w.apply(CmdAcceptMission{"mission_sky_relief"});
    CommandResult r2 = w.apply(CmdEnterVehicle{"vehicle_helo_001", "pilot"});
    step(5, "Board helicopter at Windfield", r && r2 && w.possession().role == "pilot" && w.mission("mission_sky_relief")->status == MissionStatus::Active, why(r2));
    moveTo(w, "poi_port_lyria"); w.apply(CmdLoadCargo{"economy_port_lyria", "medical_pack", 4});
    Pose air = poiPose(w, "poi_beacon_clinic"); air.position.z = 150; w.apply(CmdReportPose{"vehicle_helo_001", air});
    CommandResult airExit = w.apply(CmdExitVehicle{});
    step(5, "Exit refused while airborne", !airExit && airExit.status.code == "exit_unsafe_airborne", airExit.status.code);
    moveTo(w, "poi_beacon_clinic");
    int trust = w.region("region_asterion_west")->local_trust;
    r = w.apply(CmdDeliverCargo{"economy_beacon_clinic", "medical_pack", 4});
    moveTo(w, "poi_windfield"); w.apply(CmdExitVehicle{});
    r2 = w.apply(CmdInteractNpc{"npc_windfield_chief"});
    step(6, "Complete Sky Relief (trust/reward updated)", r && r2 && w.mission("mission_sky_relief")->status == MissionStatus::Completed && w.region("region_asterion_west")->local_trust == trust + 6, "trust " + std::to_string(trust) + " -> " + std::to_string(w.region("region_asterion_west")->local_trust));

    moveTo(w, "poi_harbor_west");
    r = w.apply(CmdAcceptMission{"mission_harbor_link"});
    r2 = w.apply(CmdEnterVehicle{"vehicle_boat_001", "captain"});
    step(7, "Travel to Harbor West, board boat", r && r2 && w.possession().role == "captain", why(r2));
    moveTo(w, "poi_port_lyria"); w.apply(CmdLoadCargo{"economy_port_lyria", "water_pack", 10});
    moveTo(w, "poi_pinewatch");
    r = w.apply(CmdDeliverCargo{"economy_pinewatch", "water_pack", 10});
    moveTo(w, "poi_harbor_west"); w.apply(CmdExitVehicle{});
    r2 = w.apply(CmdInteractNpc{"npc_harbor_master"});
    step(7, "Harbor Link delivered by sea in the same save", r && r2 && w.mission("mission_harbor_link")->status == MissionStatus::Completed, why(r));

    moveTo(w, "poi_port_lyria");
    r = w.apply(CmdAcceptMission{"mission_community_update"});
    moveTo(w, "poi_beacon_clinic"); w.apply(CmdInteractNpc{"npc_clinic_nurse"}); moveTo(w, "poi_port_lyria");
    r2 = w.apply(CmdInteractNpc{"npc_mayor_elara"});
    bool newsLinked = !w.news().empty();
    for (const auto& n : w.news()) if (!w.journal().contains(n.source_event_id)) newsLinked = false;
    step(8, "Return to town, read news (linked to committed events)", r && r2 && newsLinked && w.mission("mission_community_update")->status == MissionStatus::Completed, std::to_string(w.news().size()) + " news items");
    std::printf("     mayor: \"%s\"\n", w.npcDialogue("npc_mayor_elara").c_str());

    FileSaveStorage storage(saveDir); SaveService svc(storage);
    Status sv = svc.save(w, "acceptance");
    WorldState loaded; loaded.initFromContent(content);
    std::vector<std::string> warnings;
    Status ld = svc.load(loaded, "acceptance", &warnings);
    bool identical = ld.ok && loaded.toSnapshot().dump() == w.toSnapshot().dump();
    step(9, "Save / quit / load (all changes intact, no duplicates)", sv.ok && identical && warnings.empty(), sv.ok ? (identical ? "snapshot byte-identical" : "MISMATCH") : sv.message);

    Tick t0 = loaded.time().tick;
    SimulationReport rep = loaded.advanceTicks(24 * 60);
    step(10, "Advance world time 24h offscreen", rep.ticks_advanced == 24 * 60 && loaded.time().tick == t0 + 24 * 60,
         std::to_string(rep.committed_event_ids.size()) + " aggregate events, tierR=" + std::to_string(rep.tier_r_steps) + " tierG=" + std::to_string(rep.tier_g_steps));

    std::printf("\n=== journal (%zu events) ===\n", loaded.journal().size());
    for (const auto& e : loaded.journal().events())
        std::printf("%-22s t=%-5lld %-28s src=%s\n", e.event_id.c_str(), static_cast<long long>(e.occurred_at_world_tick), eventTypeName(e.event_type), e.source_entity_id.c_str());
    std::printf("\n=== news ===\n");
    for (const auto& n : loaded.news()) std::printf("t=%-5lld %s  [%s]\n", static_cast<long long>(n.tick), n.headline.c_str(), n.source_event_id.c_str());
    std::printf("\n=== debug summary ===\n%s", loaded.debugSummary().c_str());
    std::printf("\nRESULT: %s (%d failing step(s))\n", g_failures ? "FAIL" : "PASS", g_failures);
    return g_failures ? 1 : 0;
}
