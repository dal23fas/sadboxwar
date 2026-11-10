// Deterministic, named random streams (TDD §12 determinism).
// rng(seed, "weather:region_x", tick) always yields the same value for the same inputs,
// so simulation does not change after save/load.
#pragma once
#include <cstdint>
#include <string>

namespace ef {

inline uint64_t fnv1a64(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

inline uint64_t splitmix64(uint64_t& state) {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Returns a double in [0, 1).
inline double streamUnit(uint64_t worldSeed, const std::string& streamName, uint64_t tick) {
    uint64_t state = worldSeed ^ fnv1a64(streamName) ^ (tick * 0xD1B54A32D192ED03ULL);
    uint64_t r = splitmix64(state);
    return static_cast<double>(r >> 11) * (1.0 / 9007199254740992.0);
}

} // namespace ef
