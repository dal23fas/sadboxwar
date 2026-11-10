// UNVERIFIED IN THIS ENVIRONMENT.
#include "ef_world.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

EFWorld::EFWorld() {
    // Default save dir: user://efsaves  (Godot resolves user:// per platform)
    save_dir_ = ProjectSettings::get_singleton()->globalize_path("user://efsaves");
    api_ = std::make_unique<ef::JsonApi>(std::string(save_dir_.utf8().get_data()));
}

void EFWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("request", "request_json"), &EFWorld::request);
    ClassDB::bind_method(D_METHOD("set_save_dir", "dir"), &EFWorld::set_save_dir);
    ClassDB::bind_method(D_METHOD("get_save_dir"), &EFWorld::get_save_dir);
    ClassDB::bind_method(D_METHOD("core_version"), &EFWorld::core_version);
}

String EFWorld::request(const String& request_json) {
    std::string reply = api_->handleText(std::string(request_json.utf8().get_data()));
    return String::utf8(reply.c_str(), static_cast<int>(reply.size()));
}

void EFWorld::set_save_dir(const String& dir) {
    // Re-creating the API would drop the world; only allowed before the world is ready.
    if (api_ && api_->ready()) return;
    save_dir_ = dir;
    api_ = std::make_unique<ef::JsonApi>(std::string(save_dir_.utf8().get_data()));
}

String EFWorld::get_save_dir() const { return save_dir_; }

String EFWorld::core_version() const {
    return String("EFWorldCore save_schema=") + String::num_int64(ef::kSaveSchemaVersion) + " event_schema=" + String::num_int64(ef::kEventSchemaVersion);
}
