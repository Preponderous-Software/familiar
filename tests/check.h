// A minimal test harness: CHECK records a failure and keeps going, and main()
// returns the failure count, so ctest reports every broken expectation at once.
#pragma once

#include <cstdio>

namespace familiar_test {
inline int failures = 0;
inline int checks = 0;
}  // namespace familiar_test

#define CHECK(cond)                                                                 \
    do {                                                                            \
        ++familiar_test::checks;                                                    \
        if (!(cond)) {                                                              \
            ++familiar_test::failures;                                              \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                           \
    } while (0)

#define TEST_MAIN_END()                                                                       \
    std::printf("%d checks, %d failed\n", familiar_test::checks, familiar_test::failures);  \
    return familiar_test::failures == 0 ? 0 : 1
