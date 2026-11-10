#include "ef_test.h"
#include "EFWorldCore/Json.h"
using namespace ef;

EF_TEST(json_roundtrip_preserves_structure_and_order) {
    Json j = Json::object();
    j.set("b", 1); j.set("a", "text \"quoted\" \\ \n"); j.set("list", Json::array());
    Json arr = Json::array(); arr.push(1.5); arr.push(true); arr.push(Json()); arr.push(-7);
    j.set("list", arr);
    Json nested = Json::object(); nested.set("k", "v"); j.set("nested", nested);
    std::string text = j.dump(2);
    Json back; std::string err;
    EF_REQUIRE(Json::parse(text, back, &err));
    EF_CHECK_EQ(back.asObject()[0].first, std::string("b"));   // insertion order kept
    EF_CHECK_EQ(back.get("a").asString(), std::string("text \"quoted\" \\ \n"));
    EF_CHECK_EQ(back.get("list").size(), static_cast<size_t>(4));
    EF_CHECK_NEAR(back.get("list").asArray()[0].asNumber(), 1.5, 1e-12);
    EF_CHECK(back.get("list").asArray()[2].isNull());
    EF_CHECK_EQ(back.get("list").asArray()[3].asInt(), static_cast<int64_t>(-7));
    EF_CHECK_EQ(back.get("nested").get("k").asString(), std::string("v"));
    EF_CHECK_EQ(back.dump(2), text);   // deterministic output
}

EF_TEST(json_rejects_malformed_input) {
    Json out; std::string err;
    EF_CHECK(!Json::parse("{\"a\": }", out, &err));
    EF_CHECK(!Json::parse("[1, 2", out, &err));
    EF_CHECK(!Json::parse("{\"a\": 1} trailing", out, &err));
    EF_CHECK(!Json::parse("", out, &err));
    EF_CHECK(Json::parse("  {\"u\": \"\\u00e9\\u03b1\"} ", out, &err));
    EF_CHECK_EQ(out.get("u").asString(), std::string("\xC3\xA9\xCE\xB1"));
}

EF_TEST(json_integers_print_without_decimals) {
    Json j = Json::object(); j.set("tick", static_cast<int64_t>(8742)); j.set("ratio", 0.25);
    EF_CHECK_EQ(j.dump(), std::string("{\"tick\":8742,\"ratio\":0.25}"));
}

EF_TEST_MAIN()
