// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : runner                              Owner : Mathew Nguithi
//    ori_tests            run every test
//    ori_tests fold       run the tests whose name contains "fold"
// ============================================================================
#include "TestFramework.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    auto& cases = ori_test::registry();
    std::stable_sort(cases.begin(), cases.end(), [](const ori_test::Case& a, const ori_test::Case& b) {
        return std::strcmp(a.file, b.file) < 0;
    });
    const std::string filter = argc > 1 ? argv[1] : "";
    auto& c = ori_test::counters();
    int run = 0, failedCases = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (const ori_test::Case& t : cases) {
        if (!filter.empty() && std::string(t.name).find(filter) == std::string::npos) continue;
        c.caseFailures = 0;
        const long before = c.checks;
        t.fn();
        ++run;
        const bool ok = c.caseFailures == 0;
        failedCases += !ok;
        std::printf("  %s %-52s %6ld checks\n", ok ? "PASS" : "FAIL", t.name, c.checks - before);
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("\n%d tests, %ld checks, %d failed (%ld failing checks) in %.0f ms\n", run, c.checks, failedCases,
                c.failures, ms);
    if (run == 0) {
        std::printf("no test matches '%s'\n", filter.c_str());
        return 1;
    }
    return failedCases == 0 ? 0 : 1;
}
