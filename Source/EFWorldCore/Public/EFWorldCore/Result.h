// EFWorldCore — engine-neutral authoritative world state for EARTH FRONTIER.
// Builds with -fno-exceptions -fno-rtti (UnrealBuildTool defaults). No UE headers here.
#pragma once
#include <string>
#include <utility>

namespace ef {

// Status of a command / operation. Commands may be rejected with a reason
// without mutating world state (TDD §7).
struct Status {
    bool ok = true;
    std::string code;     // machine-readable, e.g. "mission_not_available"
    std::string message;  // human-readable

    static Status Ok() { return Status{}; }
    static Status Fail(std::string c, std::string m = {}) {
        Status s; s.ok = false; s.code = std::move(c); s.message = std::move(m); return s;
    }
    explicit operator bool() const { return ok; }
};

} // namespace ef
