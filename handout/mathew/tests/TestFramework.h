// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : a dependency-free unit-test framework     Owner : Mathew Nguithi
//
//    TEST(name) { CHECK(cond); CHECK_NEAR(a, b, eps); }
//
//  Tests register themselves at start-up; tests/test_main.cpp runs them.
//  Every CHECK is counted, failures print file:line and the expression.
// ============================================================================
#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ori_test {

struct Case {
    const char* name;
    void (*fn)();
    const char* file;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

struct Registrar {
    Registrar(const char* name, void (*fn)(), const char* file) { registry().push_back({name, fn, file}); }
};

struct Counters {
    long checks = 0;
    long failures = 0;          // in the whole run
    long caseFailures = 0;      // in the current test
};

inline Counters& counters() {
    static Counters c;
    return c;
}

inline void check(bool ok, const char* expr, const char* file, int line) {
    Counters& c = counters();
    ++c.checks;
    if (ok) return;
    ++c.failures;
    if (++c.caseFailures <= 8) std::printf("    FAILED %s:%d  CHECK(%s)\n", file, line, expr);
}

inline void checkNear(double a, double b, double eps, const char* ea, const char* eb, const char* file, int line) {
    Counters& c = counters();
    ++c.checks;
    if (std::fabs(a - b) <= eps) return;
    ++c.failures;
    if (++c.caseFailures <= 8)
        std::printf("    FAILED %s:%d  CHECK_NEAR(%s, %s): %.10g vs %.10g (eps %.3g)\n", file, line, ea, eb, a, b, eps);
}

} // namespace ori_test

#define TEST(name)                                                                  \
    static void name();                                                             \
    static ::ori_test::Registrar name##_registrar(#name, name, __FILE__);           \
    static void name()

#define CHECK(cond) ::ori_test::check(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, eps) ::ori_test::checkNear((a), (b), (eps), #a, #b, __FILE__, __LINE__)
