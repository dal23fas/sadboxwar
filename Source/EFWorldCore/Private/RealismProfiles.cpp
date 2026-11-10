#include "EFWorldCore/RealismProfiles.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace ef {

Json ResolvedProfile::toJson() const {
    Json j = Json::object();
    j.set("category", category); j.set("preset_label", preset_label); j.set("base_preset", base_preset);
    Json v = Json::object(); for (const auto& kv : values) v.set(kv.first, kv.second); j.set("values", v);
    Json o = Json::array(); for (const auto& p : overridden) o.push(p); j.set("overridden", o);
    return j;
}

bool RealismProfileManager::knownPreset(const std::string& p) const { return std::find(presetNames_.begin(), presetNames_.end(), p) != presetNames_.end(); }

Status RealismProfileManager::loadPresets(const Json& j) {
    if (!j.isObject()) return Status::Fail("presets_invalid", "root is not an object");
    if (j.get("schema_version").asInt(1) != 1) return Status::Fail("presets_invalid", "unsupported schema_version");
    std::vector<std::string> names; for (const auto& n : j.get("preset_names").asArray()) names.push_back(n.asString());
    if (names.empty()) return Status::Fail("presets_invalid", "preset_names empty");
    std::map<std::string, std::map<std::string, std::map<std::string, int>>> presets;
    for (const auto& cat : j.get("categories").asObject()) {
        for (const auto& name : names) {
            const Json& values = cat.second.get(name);
            if (!values.isObject()) return Status::Fail("presets_invalid", "category " + cat.first + " lacks preset " + name);
            for (const auto& kv : values.asObject()) {
                int v = static_cast<int>(kv.second.asInt(-1));
                if (!kv.second.isNumber() || v < 0 || v > 100) return Status::Fail("presets_invalid", cat.first + "." + name + "." + kv.first + " out of range 0..100");
                presets[cat.first][name][kv.first] = v;
            }
        }
        // every preset of a category must define the same parameter set (so CUSTOM overrides are well-defined)
        const auto& ref = presets[cat.first][names[0]];
        for (const auto& name : names) if (presets[cat.first][name].size() != ref.size()) return Status::Fail("presets_invalid", "category " + cat.first + " presets differ in parameter set");
    }
    if (presets.empty()) return Status::Fail("presets_invalid", "no categories");
    std::string def = j.get("default_global_preset").asString();
    if (std::find(names.begin(), names.end(), def) == names.end()) def = names[0];
    presetNames_ = names; presets_ = presets; defaultGlobal_ = def;
    descriptions_ = j.get("parameter_descriptions").isObject() ? j.get("parameter_descriptions") : Json::object();
    combatLevels_.clear(); for (const auto& l : j.get("combat_challenge_levels").asArray()) combatLevels_.push_back(l.asString());
    hudLevels_.clear(); for (const auto& l : j.get("hud_assist_levels").asArray()) hudLevels_.push_back(l.asString());
    if (combatLevels_.empty()) combatLevels_ = {"EASY", "NORMAL", "HARD", "CUSTOM"};
    if (hudLevels_.empty()) hudLevels_ = {"MINIMAL", "STANDARD", "FULL"};
    loaded_ = true;
    resetAll();
    return Status::Ok();
}

Status RealismProfileManager::loadPresetsFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return Status::Fail("presets_file_missing", path);
    std::stringstream ss; ss << in.rdbuf();
    Json j; std::string err;
    if (!Json::parse(ss.str(), j, &err)) return Status::Fail("presets_parse_error", err);
    return loadPresets(j);
}

Status RealismProfileManager::setGlobalPreset(const std::string& preset) {
    if (!loaded_) return Status::Fail("presets_not_loaded");
    if (!knownPreset(preset)) return Status::Fail("unknown_preset", preset);
    globalPreset_ = preset; categoryPresets_.clear(); overrides_.clear();
    return Status::Ok();
}

Status RealismProfileManager::setCategoryPreset(const std::string& category, const std::string& preset) {
    if (!loaded_) return Status::Fail("presets_not_loaded");
    if (!presets_.count(category)) return Status::Fail("unknown_category", category);
    if (!knownPreset(preset)) return Status::Fail("unknown_preset", preset);
    categoryPresets_[category] = preset; overrides_.erase(category);
    return Status::Ok();
}

Status RealismProfileManager::setOverride(const std::string& category, const std::string& parameter, int value) {
    if (!loaded_) return Status::Fail("presets_not_loaded");
    auto cit = presets_.find(category);
    if (cit == presets_.end()) return Status::Fail("unknown_category", category);
    const auto& base = cit->second.begin()->second;
    if (!base.count(parameter)) return Status::Fail("unknown_parameter", category + "." + parameter);
    if (value < 0 || value > 100) return Status::Fail("value_out_of_range", "0..100");
    overrides_[category][parameter] = value;
    return Status::Ok();
}

Status RealismProfileManager::resetCategory(const std::string& category) {
    if (!presets_.count(category)) return Status::Fail("unknown_category", category);
    categoryPresets_.erase(category); overrides_.erase(category);
    return Status::Ok();
}

void RealismProfileManager::resetAll() {
    globalPreset_ = defaultGlobal_; categoryPresets_.clear(); overrides_.clear();
    combatChallenge_ = "NORMAL"; hudAssist_ = "STANDARD"; accessibility_ = Json::object();
}

Status RealismProfileManager::setCombatChallenge(const std::string& level) {
    if (std::find(combatLevels_.begin(), combatLevels_.end(), level) == combatLevels_.end()) return Status::Fail("unknown_combat_level", level);
    combatChallenge_ = level; return Status::Ok();
}
Status RealismProfileManager::setHudAssist(const std::string& level) {
    if (std::find(hudLevels_.begin(), hudLevels_.end(), level) == hudLevels_.end()) return Status::Fail("unknown_hud_level", level);
    hudAssist_ = level; return Status::Ok();
}
Status RealismProfileManager::setAccessibility(const std::string& key, const Json& value) {
    if (key.empty()) return Status::Fail("invalid_key");
    accessibility_.set(key, value); return Status::Ok();
}

ResolvedProfile RealismProfileManager::resolve(const std::string& category) const {
    ResolvedProfile r; r.category = category;
    auto cit = presets_.find(category);
    if (cit == presets_.end()) { r.preset_label = "UNKNOWN"; return r; }
    auto cp = categoryPresets_.find(category);
    r.base_preset = cp == categoryPresets_.end() ? globalPreset_ : cp->second;
    auto pit = cit->second.find(r.base_preset);
    if (pit != cit->second.end()) r.values = pit->second;
    auto oit = overrides_.find(category);
    if (oit != overrides_.end() && !oit->second.empty()) {
        for (const auto& kv : oit->second) { r.values[kv.first] = kv.second; r.overridden.push_back(kv.first); }
        r.preset_label = "CUSTOM";
    } else {
        r.preset_label = r.base_preset;
    }
    return r;
}

std::vector<std::string> RealismProfileManager::categories() const { std::vector<std::string> v; for (const auto& kv : presets_) v.push_back(kv.first); return v; }

Json RealismProfileManager::settingsToJson() const {
    Json j = Json::object();
    j.set("schema_version", kRealismSettingsSchemaVersion);
    j.set("global_preset", globalPreset_);
    Json cp = Json::object(); for (const auto& kv : categoryPresets_) cp.set(kv.first, kv.second); j.set("category_presets", cp);
    Json ov = Json::object();
    for (const auto& kv : overrides_) { Json o = Json::object(); for (const auto& p : kv.second) o.set(p.first, p.second); ov.set(kv.first, o); }
    j.set("custom_overrides", ov);
    j.set("combat_challenge", combatChallenge_);
    j.set("hud_assist", hudAssist_);
    j.set("accessibility", accessibility_);
    return j;
}

Status RealismProfileManager::settingsFromJson(const Json& j) {
    if (!loaded_) return Status::Fail("presets_not_loaded");
    if (!j.isObject()) return Status::Fail("settings_invalid", "not an object");
    if (j.get("schema_version").asInt(1) != kRealismSettingsSchemaVersion) return Status::Fail("settings_schema_unsupported");
    RealismProfileManager tmp = *this; tmp.resetAll();
    Status s = tmp.setGlobalPreset(j.get("global_preset").asString().empty() ? defaultGlobal_ : j.get("global_preset").asString());
    if (!s) return s;
    for (const auto& kv : j.get("category_presets").asObject()) { s = tmp.setCategoryPreset(kv.first, kv.second.asString()); if (!s) return s; }
    for (const auto& cat : j.get("custom_overrides").asObject())
        for (const auto& p : cat.second.asObject()) { s = tmp.setOverride(cat.first, p.first, static_cast<int>(p.second.asInt(-1))); if (!s) return s; }
    if (j.has("combat_challenge")) { s = tmp.setCombatChallenge(j.get("combat_challenge").asString()); if (!s) return s; }
    if (j.has("hud_assist")) { s = tmp.setHudAssist(j.get("hud_assist").asString()); if (!s) return s; }
    tmp.accessibility_ = j.get("accessibility").isObject() ? j.get("accessibility") : Json::object();
    *this = tmp;   // all-or-nothing
    return Status::Ok();
}

Json RealismProfileManager::fullProjection() const {
    Json j = Json::object();
    j.set("settings", settingsToJson());
    Json names = Json::array(); for (const auto& n : presetNames_) names.push(n); j.set("preset_names", names);
    Json cl = Json::array(); for (const auto& n : combatLevels_) cl.push(n); j.set("combat_challenge_levels", cl);
    Json hl = Json::array(); for (const auto& n : hudLevels_) hl.push(n); j.set("hud_assist_levels", hl);
    Json resolved = Json::object(); for (const auto& c : categories()) resolved.set(c, resolve(c).toJson()); j.set("resolved", resolved);
    j.set("parameter_descriptions", descriptions_);
    return j;
}

} // namespace ef
