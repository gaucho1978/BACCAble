#ifndef HOST_TEST_REPORT_H
#define HOST_TEST_REPORT_H
#include <stdio.h>

typedef struct {
    void (*run)(void);
    const char *name;
} HostTest;
#define HOST_TEST(fn) {fn, #fn}

/* Announce planned cases before running them so an assertion leaves a useful report. */
static void host_tests_run(const char *suite, const HostTest *tests, size_t count) {
    for (size_t i = 0; i < count; ++i)
        printf("CASE PLAN %s %s\n", suite, tests[i].name);
    fflush(stdout);
    for (size_t i = 0; i < count; ++i) {
        printf("CASE RUN %s %s\n", suite, tests[i].name);
        fflush(stdout);
        tests[i].run();
        printf("CASE PASS %s %s\n", suite, tests[i].name);
        fflush(stdout);
    }
}
#endif
