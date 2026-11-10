// JsonApi — the single request/response surface between the authoritative core and any
// presentation host (Godot GDExtension, the ef_server dev bridge, tests, tools).
//
// Request:  {"id": <any>, "op": "<name>", "args": {...}}
// Response: {"id": <same>, "ok": true, "result": {...}}  |  {"id":..., "ok": false, "code": "...", "message": "..."}
//
// World ops:   start_new_game, content, state, apply, advance_ticks, save, load, dialogue, mission_board,
//              events_since, debug_summary, set_config
// Realism ops: load_presets, realism_get, realism_set_global, realism_set_category, realism_set_override,
//              realism_reset, realism_set_combat, realism_set_hud, realism_set_accessibility,
//              realism_save, realism_load   (settings file lives in <saveDir>/settings/, separate from world saves)
#pragma once
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/SaveService.h"
#include "EFWorldCore/RealismProfiles.h"
#include <memory>
#include <string>

namespace ef {

class JsonApi {
public:
    explicit JsonApi(std::string saveDir);
    Json handle(const Json& request);
    // Convenience for string transports (one request per call).
    std::string handleText(const std::string& requestJson);

    // Direct access for hosts that embed the core (GDExtension) and want typed reads.
    WorldState& world() { return world_; }
    const WorldState& world() const { return world_; }
    RealismProfileManager& realism() { return realism_; }
    bool ready() const { return ready_; }

    // Pure helpers (also used by tools)
    static Status commandFromJson(const Json& j, Command& out);
    static Json poseToJson(const Pose& p) { return p.toJson(); }
    Json stateProjection(int newsCount) const;
    Json contentProjection() const;
    Json missionBoardProjection() const;
    Json eventsSince(size_t index) const;

private:
    Json ok(const Json& id, Json result) const;
    Json fail(const Json& id, const std::string& code, const std::string& message) const;

    Json handleRealism(const Json& id, const std::string& op, const Json& a);
    WorldState world_;
    RealismProfileManager realism_;
    std::unique_ptr<FileSaveStorage> storage_;
    std::unique_ptr<FileSaveStorage> settingsStorage_;
    std::unique_ptr<SaveService> saves_;
    std::string saveDir_;
    bool ready_ = false;
};

} // namespace ef
