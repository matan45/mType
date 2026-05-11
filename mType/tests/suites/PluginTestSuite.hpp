#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Tests for the native plugin loader (MYT-289).
     *
     * All tests are in-process callback tests so CI doesn't need a pre-built
     * .dll/.so/.dylib. The plugin trampoline path is exercised by registering
     * a synthetic PluginNativeBinding that points at an in-process C function;
     * this gives full coverage of value bridging, arena lifetime, error
     * semantics, and the ABI v2 reentrancy ops without ever touching dlopen.
     */
    class PluginTestSuite : public testFramework::TestSuite
    {
    public:
        explicit PluginTestSuite() : TestSuite("Native Plugin Loader Test Suite") {}
        void setupTests() override;

    private:
        void setupRegistryTests();      // NativeRegistry::unregisterNativeFunction
        void setupCacheInvalidationTests(); // BytecodeProgram::clearNativeCacheSlots
        void setupTrampolineTests();    // value bridging through pluginNativeTrampoline
        void setupErrorTests();         // raiseError -> trampoline rethrow
        void setupReentrancyTests();    // callFunction / callMethod / list* / has*
        void setupLoaderValidationTests(); // PluginLoader path validation
        void setupNativesScriptTests(); // __plugin_load via .mt scripts
        void setupIntegrationTests();   // real LoadLibrary/dlopen via fixture DLL
    };
}
