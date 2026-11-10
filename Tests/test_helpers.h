#pragma once
#include "ef_test.h"
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/ContentLoader.h"
#include <string>

namespace efh {
using namespace ef;

inline bool newGame(WorldState& w) {
    WorldContent c;
    Status s = loadContentFromFile(EF_CONTENT_PATH, c);
    if (!s) { std::printf("content load failed: %s %s\n", s.code.c_str(), s.message.c_str()); return false; }
    s = w.initFromContent(c);
    if (!s) { std::printf("init failed: %s %s\n", s.code.c_str(), s.message.c_str()); return false; }
    return true;
}

inline Pose poiPose(const WorldState& w, const std::string& poiId) { auto it = w.content().pois.find(poiId); return it == w.content().pois.end() ? Pose{} : it->second.pose; }

// In the engine the pawn/vehicle physically moves and the presentation layer reports the pose.
// Headless tests report it directly.
inline CommandResult moveTo(WorldState& w, const std::string& poiId) {
    Pose p = poiPose(w, poiId);
    if (w.possession().mode == PossessionMode::Seated) return w.apply(CmdReportPose{w.possession().vehicle_id, p});
    return w.apply(CmdReportPose{w.player().entity_id, p});
}

inline void expectOk(const CommandResult& r, const char* what) {
    if (!r) { std::printf("  command %s rejected: %s (%s)\n", what, r.status.code.c_str(), r.status.message.c_str()); }
    EF_CHECK(r.status.ok);
}

inline size_t countEvents(const WorldState& w, EventType t) { size_t n = 0; for (const auto& e : w.journal().events()) if (e.event_type == t) ++n; return n; }

// Plays the first two slice missions on foot + truck: First Contact, Road Supplies (6 food crates to Beacon Clinic).
inline void playThroughRoadSupplies(WorldState& w) {
    expectOk(w.apply(CmdAcceptMission{"mission_first_contact"}), "accept first contact");
    expectOk(moveTo(w, "poi_harbor_west"), "walk to harbor");
    expectOk(w.apply(CmdInteractNpc{"npc_harbor_master"}), "talk harbor master");
    expectOk(moveTo(w, "poi_port_lyria"), "walk back");
    expectOk(w.apply(CmdInteractNpc{"npc_mayor_elara"}), "debrief first contact");
    expectOk(w.apply(CmdAcceptMission{"mission_road_supplies"}), "accept road supplies");
    expectOk(moveTo(w, "poi_depot_south"), "walk to depot");
    expectOk(w.apply(CmdEnterVehicle{"vehicle_truck_001", "driver"}), "enter truck");
    expectOk(moveTo(w, "poi_port_lyria"), "drive to town");
    expectOk(w.apply(CmdLoadCargo{"economy_port_lyria", "food_crate", 6}), "load food");
    expectOk(moveTo(w, "poi_beacon_clinic"), "drive to clinic");
    expectOk(w.apply(CmdDeliverCargo{"economy_beacon_clinic", "food_crate", 6}), "deliver food");
    expectOk(moveTo(w, "poi_port_lyria"), "drive back");
    expectOk(w.apply(CmdExitVehicle{}), "exit truck");
    expectOk(w.apply(CmdInteractNpc{"npc_mayor_elara"}), "debrief road supplies");
}
}
