// Minimal test harness (no external dependencies; works with -fno-exceptions).
#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace eftest {
struct Case { const char* name; std::function<void()> fn; };
inline std::vector<Case>& cases() { static std::vector<Case> c; return c; }
inline int& failures() { static int f = 0; return f; }
inline const char*& current() { static const char* c = ""; return c; }
struct Reg { Reg(const char* n, std::function<void()> f) { cases().push_back({n, std::move(f)}); } };
inline void fail(const char* file, int line, const std::string& msg) {
    ++failures();
    std::printf("  FAIL [%s] %s:%d: %s\n", current(), file, line, msg.c_str());
}
inline int runAll() {
    int ran = 0;
    for (auto& c : cases()) { current() = c.name; int before = failures(); c.fn(); ++ran; std::printf("%s %s\n", failures() == before ? "PASS" : "FAIL", c.name); }
    std::printf("%d test(s), %d failure(s)\n", ran, failures());
    return failures() ? 1 : 0;
}
}

#define EF_TEST(name) static void name(); static eftest::Reg name##_reg(#name, name); static void name()
#define EF_CHECK(cond) do { if (!(cond)) eftest::fail(__FILE__, __LINE__, #cond); } while (0)
#define EF_CHECK_EQ(a, b) do { auto _a = (a); auto _b = (b); if (!(_a == _b)) eftest::fail(__FILE__, __LINE__, std::string(#a " == " #b) + "  (got " + eftest::str(_a) + " vs " + eftest::str(_b) + ")"); } while (0)
#define EF_CHECK_NEAR(a, b, eps) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) > (eps)) eftest::fail(__FILE__, __LINE__, std::string(#a " ~= " #b) + " (got " + std::to_string(_a) + " vs " + std::to_string(_b) + ")"); } while (0)
#define EF_REQUIRE(cond) do { if (!(cond)) { eftest::fail(__FILE__, __LINE__, std::string("REQUIRE ") + #cond); return; } } while (0)
#define EF_TEST_MAIN() int main() { return eftest::runAll(); }

namespace eftest {
inline std::string str(const std::string& s) { return "\"" + s + "\""; }
inline std::string str(const char* s) { return std::string("\"") + s + "\""; }
inline std::string str(bool b) { return b ? "true" : "false"; }
template <class T> inline std::string str(const T& v) { return std::to_string(v); }
}
