// Multi-rate world simulation (master prompt §06, §40; World Sim §2).
// Tier L (local physics/animation) lives in the engine. Tiers R and G live here and
// run identically with or without 3D presentation (WORLD-01 / WS-06).
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/Rng.h"
#include <algorithm>
#include <cmath>

namespace ef {

SimulationReport WorldState::advanceTicks(Tick ticks) {
    SimulationReport rep;
    if (!initialized_ || ticks <= 0) return rep;
    Tick toRun = std::min(ticks, config_.max_catchup_ticks_per_call);   // bounded catch-up (TDD §8)
    rep.ticks_remaining = ticks - toRun;
    for (Tick i = 0; i < toRun; ++i) {
        ++time_.tick;
        ++rep.ticks_advanced;
        if (config_.tier_r_interval_ticks > 0 && time_.tick % config_.tier_r_interval_ticks == 0) { stepTierR(rep.committed_event_ids); ++rep.tier_r_steps; }
        if (config_.tier_g_interval_ticks > 0 && time_.tick % config_.tier_g_interval_ticks == 0) { stepTierG(rep.committed_event_ids); ++rep.tier_g_steps; }
    }
    return rep;
}

// Tier R: aggregate production/consumption per economy node.
// stock_next = clamp(stock + production - consumption, 0, capacity)   (World Sim §8)
void WorldState::stepTierR(std::vector<std::string>& committed) {
    std::vector<EventEnvelope> events;
    for (const auto& kv : economy_) {
        const EconomyNodeState& n = kv.second;
        std::set<std::string> items;
        for (const auto& p : n.production_per_step) items.insert(p.first);
        for (const auto& c : n.consumption_per_step) items.insert(c.first);
        for (const auto& item : items) {
            int prod = n.production_per_step.count(item) ? n.production_per_step.at(item) : 0;
            int cons = n.consumption_per_step.count(item) ? n.consumption_per_step.at(item) : 0;
            int cap = n.capacityOf(item);
            int cur = n.stock(item);
            int next = cur + prod - cons;
            next = std::max(0, cap > 0 ? std::min(next, cap) : next);
            if (next == cur) continue;
            Json pl = Json::object();
            pl.set("node_id", n.id); pl.set("item_id", item); pl.set("previous_quantity", cur); pl.set("new_quantity", next);
            pl.set("delta", next - cur); pl.set("reason_tag", "tier_r_production_consumption");
            events.push_back(makeEvent(EventType::ResourceStockChanged, "system_economy", n.region_id, pl));
        }
    }
    for (const auto& e : events) commitWithReactions(e, committed);
}

// Tier G: weather (seeded, deterministic per region+tick) and civic stability drift.
void WorldState::stepTierG(std::vector<std::string>& committed) {
    std::vector<EventEnvelope> events;
    for (const auto& kv : weather_) {
        const WeatherState& w = kv.second;
        double r = streamUnit(seed_, "weather:" + kv.first, static_cast<uint64_t>(time_.tick));
        std::string next = w.condition;
        if (w.condition == "clear")      { if (r < 0.04) next = "storm"; else if (r < 0.18) next = "rain"; }
        else if (w.condition == "rain")  { if (r < 0.10) next = "storm"; else if (r < 0.55) next = "clear"; }
        else /* storm */                 { if (r < 0.65) next = "rain"; }
        if (next == w.condition) continue;
        Json pl = Json::object(); pl.set("region_id", kv.first); pl.set("previous", w.condition); pl.set("new", next);
        pl.set("stream", "weather:" + kv.first);
        events.push_back(makeEvent(EventType::WeatherChanged, "system_weather", kv.first, pl));
    }
    for (const auto& kv : regions_) {
        const RegionState& r = kv.second;
        // mean shortage of civilian supplies across the region's nodes
        double sum = 0; int count = 0;
        for (const auto& nid : r.economy_node_ids) {
            const EconomyNodeState* n = economyNode(nid); if (!n) continue;
            for (const auto& st : n->stocks) {
                auto it = content_.items.find(st.first);
                if (it == content_.items.end() || it->second.category != "civilian_supplies") continue;
                sum += n->shortage(st.first); ++count;
            }
        }
        double meanShortage = count ? sum / count : 0.0;
        double target = meanShortage > 0.6 ? r.stability_normalized - 0.01 : std::min(0.95, r.stability_normalized + 0.005);
        target = std::round(std::clamp(target, 0.0, 1.0) * 10000.0) / 10000.0;
        if (std::fabs(target - r.stability_normalized) < 1e-9) continue;
        Json pl = Json::object(); pl.set("region_id", kv.first); pl.set("previous", r.stability_normalized); pl.set("new", target);
        pl.set("mean_shortage", meanShortage); pl.set("reason_tag", meanShortage > 0.6 ? "supply_shortage" : "recovery");
        events.push_back(makeEvent(EventType::RegionStabilityChanged, "system_simulation_director", kv.first, pl));
    }
    for (const auto& e : events) commitWithReactions(e, committed);
}

} // namespace ef
