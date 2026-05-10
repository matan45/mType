/*
 * Tiny test-fixture plugin for the PluginTestSuite integration test (MYT-289).
 *
 * Built as a shared library by the premake `plugin-test-fixture` project.
 * Output lands at mType/tests/testFiles/plugin/plugin_test_fixture.<ext>
 * which the integration test loads via the real PluginLoader (LoadLibraryW
 * / dlopen) to validate the OS-boundary code paths the in-process unit
 * tests bypass.
 *
 * IMPORTANT: this TU includes ONLY PluginHostApi.h. It must not pull in any
 * engine headers — the whole point of the fixture is to exercise the C-ABI
 * boundary as a third-party plugin would.
 */

#include "PluginHostApi.h"

namespace {
const MTypePluginHost* g_host = nullptr;

MTypeValue* fixturePlusSeven(void* /*userData*/, MTypeContext* ctx,
                              const MTypeValue* const* args, int argc)
{
    if (argc != 1 || g_host->getTag(args[0]) != MT_TAG_INT) {
        g_host->raiseError(ctx, "FixtureError",
                           "__pt_fixture_plus_seven expects one int arg");
        return g_host->makeNull(ctx);
    }
    return g_host->makeInt(ctx, g_host->getInt(args[0]) + 7);
}
}  // namespace

extern "C" MTYPE_PLUGIN_EXPORT
int mtype_plugin_register(uint32_t hostAbiVersion,
                          const MTypePluginHost* host,
                          MTypeContext* registrationCtx)
{
    if (hostAbiVersion != MTYPE_PLUGIN_ABI_VERSION) {
        return 1;
    }
    g_host = host;
    host->registerFunction(registrationCtx,
                           "__pt_fixture_plus_seven",
                           &fixturePlusSeven,
                           nullptr);
    return 0;
}
