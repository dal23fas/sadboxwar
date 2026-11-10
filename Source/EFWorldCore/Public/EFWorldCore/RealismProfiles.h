// RealismProfileManager (supplement §1–3, §11–12).
// Resolves handling-assist parameters per vehicle/character category from:
//   global preset  →  per-category preset override  →  per-parameter custom sliders (0–100)
// Settings live in a SEPARATE file from world saves (supplement §12) and never influence
// WorldState rules: the core has no dependency on this module (tested: same commands → same events).
#pragma once
#include "EFWorldCore/Json.h"
#include "EFWorldCore/Result.h"
#include <map>
#include <string>
#include <vector>

namespace ef {

constexpr int kRealismSettingsSchemaVersion = 1;

struct ResolvedProfile {
    std::string category;
    std::string preset_label;                 // ARCADE | BALANCED | REALISTIC | CUSTOM
    std::string base_preset;                  // preset the values derive from
    std::map<std::string, int> values;        // parameter -> 0..100
    std::vector<std::string> overridden;      // parameters changed by custom sliders
    Json toJson() const;
};

class RealismProfileManager {
public:
    // Load preset definitions (Content/Data/realism_presets_v1.json).
    Status loadPresets(const Json& presets);
    Status loadPresetsFromFile(const std::string& path);
    bool loaded() const { return loaded_; }

    // ---- settings mutations (all validated; invalid input leaves settings unchanged) ----
    Status setGlobalPreset(const std::string& preset);               // sets defaults: clears category presets + overrides
    Status setCategoryPreset(const std::string& category, const std::string& preset);   // clears that category's overrides
    Status setOverride(const std::string& category, const std::string& parameter, int value);   // category becomes CUSTOM
    Status resetCategory(const std::string& category);               // back to following the global preset exactly
    void resetAll();                                                 // T16: exact global preset defaults
    Status setCombatChallenge(const std::string& level);
    Status setHudAssist(const std::string& level);
    Status setAccessibility(const std::string& key, const Json& value);

    // ---- queries ----
    ResolvedProfile resolve(const std::string& category) const;
    std::vector<std::string> categories() const;
    std::vector<std::string> presetNames() const { return presetNames_; }
    const std::string& globalPreset() const { return globalPreset_; }
    const std::string& combatChallenge() const { return combatChallenge_; }
    const std::string& hudAssist() const { return hudAssist_; }
    Json parameterDescriptions() const { return descriptions_; }

    // ---- persistence (settings document; separate from world saves) ----
    Json settingsToJson() const;
    Status settingsFromJson(const Json& j);
    Json fullProjection() const;   // settings + resolved profiles for every category (for UI)

private:
    bool loaded_ = false;
    std::vector<std::string> presetNames_;
    std::string defaultGlobal_ = "BALANCED";
    std::map<std::string, std::map<std::string, std::map<std::string, int>>> presets_;  // category -> preset -> param -> value
    Json descriptions_ = Json::object();
    std::vector<std::string> combatLevels_, hudLevels_;

    std::string globalPreset_ = "BALANCED";
    std::map<std::string, std::string> categoryPresets_;
    std::map<std::string, std::map<std::string, int>> overrides_;
    std::string combatChallenge_ = "NORMAL";
    std::string hudAssist_ = "STANDARD";
    Json accessibility_ = Json::object();

    bool knownPreset(const std::string& p) const;
};

} // namespace ef
