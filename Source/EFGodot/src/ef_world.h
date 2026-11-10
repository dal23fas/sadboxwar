// UNVERIFIED IN THIS ENVIRONMENT (godot-cpp not available offline). Thin GDExtension wrapper:
// one class, one method, the same JSON API that ef_server exposes over TCP. World.gd detects this
// class with ClassDB.class_exists("EFWorld") and prefers it over TCP.
#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include "EFWorldCore/JsonApi.h"
#include <memory>

namespace godot {

class EFWorld : public RefCounted {
    GDCLASS(EFWorld, RefCounted)

public:
    EFWorld();
    ~EFWorld() override = default;

    // Request/response in JSON text. See Source/EFWorldCore/Public/EFWorldCore/JsonApi.h for ops.
    String request(const String& request_json);
    // Where saves go (default: user://efsaves resolved by the caller; set before the first save/load).
    void set_save_dir(const String& dir);
    String get_save_dir() const;
    String core_version() const;

protected:
    static void _bind_methods();

private:
    std::unique_ptr<ef::JsonApi> api_;
    String save_dir_;
};

} // namespace godot
