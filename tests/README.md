# Host regression tests

Run `make -C tests -j2 test`. Assertions and ASan/UBSan failures stop the test run
and fail CI. The cases run in their existing order, once per executable.

In GitHub, open **Actions → workflow run → Summary → Host regression scenarios**.
The table lists scenario results and distinguishes 18/24-character menu/catalog
builds and C2/BH parking builds. Download the **host-test-log** artifact and open
`host-tests.html` for an offline table and expandable full build/test log.

- **PASS**: the case returned successfully.
- **INCOMPLETE**: it started but did not finish; inspect the assertion/sanitizer log.
- **NOT RUN**: announced by the executable but not reached after an earlier failure.

Suites that never started (for example after a compiler error) have no scenario
rows. The report shows the overall test-step outcome; a partial table is not a
successful run. These host checks do not simulate physical dashboard refresh.

Register new case functions in the executable's `HostTest` array using
`HOST_TEST(function_name)`. The readable label comes from the function name.
Keep related assertions together; do not split stateful cases solely for reporting.
The existing CI publishes reports on failures too, without extra token permissions,
third-party report actions, dependencies or a second test execution.
