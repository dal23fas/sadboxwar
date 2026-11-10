// Commands = requests to mutate the world. They are validated; accepted commands
// produce events; rejected commands produce no state change (TDD §7, World Sim §6).
#pragma once
#include "EFWorldCore/Domain.h"
#include "EFWorldCore/Result.h"
#include <variant>
#include <string>
#include <vector>

namespace ef {

struct CmdInteractNpc        { EntityId npc_id; };
struct CmdAcceptMission      { EntityId mission_id; };
struct CmdEnterVehicle       { EntityId vehicle_id; std::string seat_id; };
struct CmdExitVehicle        { };
struct CmdSwitchSeat         { std::string seat_id; };
// Presentation layer reports authoritative transform at a low rate (not every frame).
struct CmdReportPose         { EntityId entity_id; Pose pose; };
// Move goods between an economy node and the player's current carrier (vehicle when seated, inventory on foot).
struct CmdLoadCargo          { EntityId node_id; std::string item_id; int quantity = 0; };
struct CmdDeliverCargo       { EntityId node_id; std::string item_id; int quantity = 0; };
// Abstract damage / repair.
struct CmdSetVehicleCondition{ EntityId vehicle_id; double condition_normalized = 1.0; std::string reason_tag; };
// System / debug / scripted weather.
struct CmdSetWeather         { EntityId region_id; std::string condition; };
struct CmdSetCampaignFlag    { std::string flag; std::string value; };

using Command = std::variant<
    CmdInteractNpc, CmdAcceptMission, CmdEnterVehicle, CmdExitVehicle, CmdSwitchSeat,
    CmdReportPose, CmdLoadCargo, CmdDeliverCargo, CmdSetVehicleCondition, CmdSetWeather,
    CmdSetCampaignFlag>;

struct CommandResult {
    Status status;
    std::vector<std::string> committed_event_ids;   // includes derived events
    explicit operator bool() const { return status.ok; }
};

} // namespace ef
