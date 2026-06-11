/*
 * Deliberately-broken fixture for PluginTestSuite: a perfectly valid shared
 * library that does NOT export mtype_plugin_register. The loader must fail
 * with a clear entry-point error and must not leave the library tracked as
 * loaded. Built by the premake `plugin-test-fixture-noentry` project to
 * mType/tests/testFiles/plugin/plugin_test_fixture_noentry.<ext>.
 *
 * Like fixture.cpp, this TU must not pull in any engine headers.
 */

#if defined(_WIN32)
#  define NOENTRY_EXPORT __declspec(dllexport)
#else
#  define NOENTRY_EXPORT __attribute__((visibility("default")))
#endif

extern "C" NOENTRY_EXPORT int mtype_plugin_definitely_not_the_entry(void)
{
    return 42;
}
