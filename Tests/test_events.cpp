// EVT-01 / WS-01: duplicate event IDs apply once. Event validation contract (World Sim §5).
#include "ef_test.h"
#include "EFWorldCore/Events.h"
#include "EFWorldCore/WorldState.h"
#include "EFWorldCore/ContentLoader.h"
using namespace ef;

static EventEnvelope sampleEvent(const std::string& id) {
    EventEnvelope e; e.event_id = id; e.event_type = EventType::WeatherChanged; e.occurred_at_world_tick = 10;
    e.source_entity_id = "system_weather"; e.region_id = "region_asterion_west";
    e.payload.set("region_id", "region_asterion_west"); e.payload.set("previous", "clear"); e.payload.set("new", "storm");
    return e;
}

EF_TEST(journal_dedups_event_ids) {
    EventJournal j;
    EF_CHECK(j.append(sampleEvent("evt_1")));
    EF_CHECK(!j.append(sampleEvent("evt_1")));
    EF_CHECK(j.append(sampleEvent("evt_2")));
    EF_CHECK_EQ(j.size(), static_cast<size_t>(2));
    Json saved = j.toJson();
    EventJournal back; std::string err;
    EF_REQUIRE(back.fromJson(saved, &err));
    EF_CHECK_EQ(back.size(), static_cast<size_t>(2));
    EF_CHECK(back.contains("evt_2"));
}

EF_TEST(envelope_validation_rejects_malformed) {
    EventEnvelope e = sampleEvent("evt_x");
    EF_CHECK(e.validate().ok);
    e.occurred_at_world_tick = -1; EF_CHECK(!e.validate().ok);
    e = sampleEvent("evt_x"); e.event_type = EventType::Unknown; EF_CHECK(!e.validate().ok);
    e = sampleEvent(""); EF_CHECK(!e.validate().ok);
    e = sampleEvent("evt_x"); e.schema_version = 99; EF_CHECK(!e.validate().ok);
    Json j = sampleEvent("evt_y").toJson(); EventEnvelope out; std::string err;
    EF_CHECK(EventEnvelope::fromJson(j, out, &err));
    EF_CHECK_EQ(std::string(eventTypeName(out.event_type)), std::string("WeatherChanged"));
    j.set("event_type", "NotAnEvent");
    EF_CHECK(!EventEnvelope::fromJson(j, out, &err));
}

// WS-01: ingesting the same event twice mutates the world once (including derived effects).
EF_TEST(world_ingest_duplicate_event_applies_once) {
    WorldContent c; EF_REQUIRE(loadContentFromFile(EF_CONTENT_PATH, c).ok);
    WorldState w; EF_REQUIRE(w.initFromContent(c).ok);
    EF_CHECK_EQ(w.regionWeather("region_asterion_west")->condition, std::string("clear"));
    bool applied = false;
    EF_CHECK(w.ingestEvent(sampleEvent("ext_evt_1"), &applied).ok); EF_CHECK(applied);
    size_t journalAfterFirst = w.journal().size();
    size_t newsAfterFirst = w.news().size();
    EF_CHECK_EQ(w.regionWeather("region_asterion_west")->condition, std::string("storm"));
    EF_CHECK_EQ(w.infrastructureState("port_harbor_west")->status, std::string("restricted"));   // derived
    EF_CHECK(newsAfterFirst >= 2);   // weather news + infrastructure news
    EF_CHECK(w.ingestEvent(sampleEvent("ext_evt_1"), &applied).ok); EF_CHECK(!applied);
    EF_CHECK_EQ(w.journal().size(), journalAfterFirst);
    EF_CHECK_EQ(w.news().size(), newsAfterFirst);
    // malformed event rejected without mutation
    EventEnvelope bad = sampleEvent("ext_evt_2"); bad.occurred_at_world_tick = -5;
    EF_CHECK(!w.ingestEvent(bad, &applied).ok); EF_CHECK(!applied);
    EF_CHECK_EQ(w.journal().size(), journalAfterFirst);
}

EF_TEST_MAIN()
