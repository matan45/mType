#include "PluginTestSuite.hpp"

#include "../../environment/EnvironmentBuilder.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/NativeContext.hpp"
#include "../../environment/registry/NativeRegistry.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/registry/FunctionRegistry.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/InternedString.hpp"
#include "../../value/NativeArray.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../errors/RuntimeException.hpp"

#include "../../plugin/PluginHostApi.h"
#include "../../plugin/PluginHost.hpp"
#include "../../plugin/PluginLoader.hpp"
#include "../../plugin/PluginNatives.hpp"

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond) throw std::runtime_error(msg);
        }

        /* Invoke a registered NativeDelegate the way the VM does. */
        ::value::Value invokeNative(
            const ::environment::registry::NativeFunction& fn,
            std::shared_ptr<::environment::Environment> env,
            std::shared_ptr<::vm::runtime::VirtualMachine> vm,
            std::initializer_list<::value::Value> args)
        {
            ::environment::NativeContext ctx{ std::move(env), std::move(vm) };
            std::vector<::value::Value> argsVec(args);
            return fn(ctx, std::span<const ::value::Value>(argsVec.data(), argsVec.size()));
        }

        /* Register a synthetic plugin-style native into the engine's registry.
         * Uses the real plugin trampoline (so all v1+v2 ABI features run end to
         * end) but without touching dlopen. The binding is owned by the
         * returned unique_ptr and must outlive any call to the registered name. */
        std::unique_ptr<::plugin::PluginNativeBinding>
        installSyntheticPlugin(std::shared_ptr<::environment::Environment> env,
                                const std::string& name,
                                ::MTypeNativeFn fn,
                                void* userData = nullptr)
        {
            auto binding = std::make_unique<::plugin::PluginNativeBinding>();
            binding->fn = fn;
            binding->userData = userData;
            binding->owner = nullptr;
            binding->name = name;

            ::environment::registry::NativeDelegate d{};
            d.userData = binding.get();
            d.invoke = [](void* u, ::environment::NativeContext& nc,
                          std::span<const ::value::Value> args) {
                return ::plugin::pluginNativeTrampoline(u, nc, args);
            };
            env->getNativeRegistry()->registerNativeFunction(name, d);
            return binding;
        }
    }

    void PluginTestSuite::setupTests()
    {
        setupRegistryTests();
        setupCacheInvalidationTests();
        setupTrampolineTests();
        setupErrorTests();
        setupReentrancyTests();
        setupLoaderValidationTests();
        setupIntegrationTests();
    }

    /* ============================================================
     * NativeRegistry::unregisterNativeFunction
     * ============================================================ */

    void PluginTestSuite::setupRegistryTests()
    {
        addCallbackTest("NativeRegistry::unregisterNativeFunction removes the entry",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto registry = env->getNativeRegistry();

                ::environment::registry::NativeDelegate d{};
                d.userData = nullptr;
                d.invoke = [](void*, ::environment::NativeContext&,
                              std::span<const ::value::Value>) -> ::value::Value {
                    return ::value::Value(int64_t{0});
                };
                registry->registerNativeFunction("__pt_temp", d);
                require(registry->hasNativeFunction("__pt_temp"),
                        "registered name should be present");

                bool removed = registry->unregisterNativeFunction("__pt_temp");
                require(removed, "unregister should return true on success");
                require(!registry->hasNativeFunction("__pt_temp"),
                        "name should be gone after unregister");
            });

        addCallbackTest("NativeRegistry::unregisterNativeFunction returns false for missing name",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                bool removed = env->getNativeRegistry()
                                  ->unregisterNativeFunction("__pt_does_not_exist_zz");
                require(!removed, "unregister of unknown name should return false");
            });

        addCallbackTest("Re-registering an unregistered native succeeds",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto registry = env->getNativeRegistry();

                ::environment::registry::NativeDelegate d{};
                d.userData = nullptr;
                d.invoke = [](void*, ::environment::NativeContext&,
                              std::span<const ::value::Value>) -> ::value::Value {
                    return ::value::Value(int64_t{1});
                };
                registry->registerNativeFunction("__pt_recycle", d);
                require(registry->unregisterNativeFunction("__pt_recycle"),
                        "first unregister should succeed");
                registry->registerNativeFunction("__pt_recycle", d);
                require(registry->hasNativeFunction("__pt_recycle"),
                        "re-registration should put the name back");
            });
    }

    /* ============================================================
     * BytecodeProgram::clearNativeCacheSlots
     * ============================================================ */

    void PluginTestSuite::setupCacheInvalidationTests()
    {
        addCallbackTest("clearNativeCacheSlots zeroes cachedNativeFunc but keeps other IC fields",
            "",
            [](services::ScriptAPI&) {
                ::vm::bytecode::BytecodeProgram prog;
                auto& slot = prog.getOrCreateCachedState(/*ip=*/0);

                /* Populate both fields. cachedFuncMetadata is just a non-null
                 * pointer for the round-trip — value is never dereferenced. */
                static ::vm::bytecode::BytecodeProgram::FunctionMetadata sentinel{};
                slot.cachedFuncMetadata = &sentinel;
                slot.cachedNativeFunc.userData = reinterpret_cast<void*>(uintptr_t{0xCAFEull});
                slot.cachedNativeFunc.invoke =
                    [](void*, ::environment::NativeContext&,
                       std::span<const ::value::Value>) -> ::value::Value {
                        return ::value::Value();
                    };

                require(static_cast<bool>(slot.cachedNativeFunc),
                        "precondition: cachedNativeFunc populated");

                prog.clearNativeCacheSlots();

                const auto* after = prog.findCachedState(0);
                require(after != nullptr, "slot should still exist after clear");
                require(!static_cast<bool>(after->cachedNativeFunc),
                        "cachedNativeFunc.invoke must be null after clear");
                require(after->cachedFuncMetadata == &sentinel,
                        "cachedFuncMetadata must survive the clear");
            });
    }

    /* ============================================================
     * Plugin trampoline value bridging
     * ============================================================ */

    void PluginTestSuite::setupTrampolineTests()
    {
        addCallbackTest("Plugin trampoline round-trips an int arg and return",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto plusOne = +[](void*, ::MTypeContext* ctx,
                                    const ::MTypeValue* const* args, int argc) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    if (argc != 1 || host->getTag(args[0]) != MT_TAG_INT) {
                        host->raiseError(ctx, "TestError", "expected int");
                        return host->makeNull(ctx);
                    }
                    return host->makeInt(ctx, host->getInt(args[0]) + 1);
                };
                auto binding = installSyntheticPlugin(env, "__pt_plus_one", plusOne);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_plus_one");
                ::value::Value out = invokeNative(fn, env, vm, { ::value::Value(int64_t{41}) });
                require(::value::isInt(out), "result should be int");
                require(::value::asInt(out) == 42,
                        "expected 42, got " + std::to_string(::value::asInt(out)));
            });

        addCallbackTest("Plugin trampoline round-trips a string arg and return",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto greet = +[](void*, ::MTypeContext* ctx,
                                  const ::MTypeValue* const* args, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    size_t n = 0;
                    const char* name = host->getString(args[0], &n);
                    std::string out = "hi:";
                    out.append(name, n);
                    return host->makeString(ctx, out.c_str(), out.size());
                };
                auto binding = installSyntheticPlugin(env, "__pt_greet", greet);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_greet");
                ::value::Value out = invokeNative(fn, env, vm, { ::value::Value(std::string("xx")) });
                require(::value::isString(out) || ::value::isInternedString(out),
                        "result should be a string Value");
                std::string s = ::value::isString(out)
                    ? ::value::asString(out)
                    : ::value::asInternedString(out).getString();
                require(s == "hi:xx", "expected 'hi:xx', got '" + s + "'");
            });

        addCallbackTest("Plugin trampoline returns void when plugin returns null",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto noop = +[](void*, ::MTypeContext*, const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    return nullptr;
                };
                auto binding = installSyntheticPlugin(env, "__pt_void", noop);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_void");
                ::value::Value out = invokeNative(fn, env, vm, {});
                require(::value::isVoid(out),
                        "null return should marshal as VOID Value");
            });

        addCallbackTest("makeArray + arraySet + arrayLen round-trip",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto buildArr = +[](void*, ::MTypeContext* ctx,
                                     const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* arr = host->makeArray(ctx, MT_TAG_INT, 3);
                    host->arraySet(arr, 0, host->makeInt(ctx, 10));
                    host->arraySet(arr, 1, host->makeInt(ctx, 20));
                    host->arraySet(arr, 2, host->makeInt(ctx, 30));
                    return arr;
                };
                auto binding = installSyntheticPlugin(env, "__pt_arr", buildArr);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_arr");
                ::value::Value out = invokeNative(fn, env, vm, {});
                require(::value::isNativeArray(out), "result should be a NativeArray");
                auto arr = ::value::asNativeArray(out);
                require(arr->size() == 3,
                        "array size should be 3, got " + std::to_string(arr->size()));
                require(::value::asInt(arr->get(0)) == 10, "arr[0] should be 10");
                require(::value::asInt(arr->get(1)) == 20, "arr[1] should be 20");
                require(::value::asInt(arr->get(2)) == 30, "arr[2] should be 30");
            });
    }

    /* ============================================================
     * Error semantics — raiseError -> trampoline rethrow
     * ============================================================ */

    void PluginTestSuite::setupErrorTests()
    {
        addCallbackTest("raiseError causes trampoline to throw RuntimeException",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto boom = +[](void*, ::MTypeContext* ctx,
                                 const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    host->raiseError(ctx, "TestError", "intentional failure");
                    return host->makeNull(ctx);
                };
                auto binding = installSyntheticPlugin(env, "__pt_boom", boom);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_boom");
                bool threw = false;
                std::string msg;
                try {
                    invokeNative(fn, env, vm, {});
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "trampoline should rethrow on errorPending");
                require(msg.find("TestError") != std::string::npos
                        && msg.find("intentional failure") != std::string::npos,
                        "exception should include type and message, got: " + msg);
            });
    }

    /* ============================================================
     * ABI v2 reentrancy & enumeration
     * ============================================================ */

    void PluginTestSuite::setupReentrancyTests()
    {
        addCallbackTest("hasFunction sees both natives and missing names",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto probe = +[](void*, ::MTypeContext* ctx,
                                  const ::MTypeValue* const* args, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    size_t n = 0;
                    const char* name = host->getString(args[0], &n);
                    return host->makeBool(ctx, host->hasFunction(ctx, name));
                };
                auto binding = installSyntheticPlugin(env, "__pt_has", probe);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_has");

                /* sqrt is a built-in registered by EnvironmentBuilder (math native). */
                ::value::Value found = invokeNative(fn, env, vm, { ::value::Value(std::string("sqrt")) });
                require(::value::isBool(found) && ::value::asBool(found),
                        "hasFunction('sqrt') should be true");

                ::value::Value missing = invokeNative(fn, env, vm, { ::value::Value(std::string("__pt_no_such_zz")) });
                require(::value::isBool(missing) && !::value::asBool(missing),
                        "hasFunction(unknown) should be false");
            });

        addCallbackTest("listFunctions enumerates registered names",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                /* Plugin native that counts how many names listFunctions yields. */
                auto count = +[](void*, ::MTypeContext* ctx,
                                  const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    int64_t total = 0;
                    host->listFunctions(ctx,
                        [](void* u, const char*) { ++(*static_cast<int64_t*>(u)); },
                        &total);
                    return host->makeInt(ctx, total);
                };
                auto binding = installSyntheticPlugin(env, "__pt_count", count);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_count");
                ::value::Value out = invokeNative(fn, env, vm, {});
                require(::value::isInt(out), "count should be int");
                /* Many built-in natives are registered; just sanity check >0. */
                require(::value::asInt(out) > 0,
                        "listFunctions should yield at least one name, got "
                        + std::to_string(::value::asInt(out)));
            });
    }

    /* ============================================================
     * PluginLoader path validation (no real DLL needed)
     * ============================================================ */

    void PluginTestSuite::setupLoaderValidationTests()
    {
        addCallbackTest("PluginLoader::load rejects empty path",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                bool threw = false;
                std::string msg;
                try {
                    ::plugin::PluginLoader::instance().load("", env, vm);
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "load(\"\") should throw");
                require(msg.find("empty") != std::string::npos,
                        "error should mention empty path, got: " + msg);
            });

        addCallbackTest("PluginLoader::load rejects nonexistent file",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                bool threw = false;
                std::string msg;
                try {
                    ::plugin::PluginLoader::instance().load(
                        "mType/tests/__no_such_plugin_zz.dll", env, vm);
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "load of nonexistent file should throw");
                require(msg.find("not found") != std::string::npos,
                        "error should mention 'not found', got: " + msg);
            });

        addCallbackTest("PluginLoader::unload rejects unknown path",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                bool threw = false;
                try {
                    ::plugin::PluginLoader::instance().unload(
                        "mType/tests/__never_loaded_zz.dll", env, vm);
                } catch (const ::errors::RuntimeException&) {
                    threw = true;
                }
                require(threw, "unload of unknown path should throw");
            });

        addCallbackTest("__plugin_load native registered by EnvironmentBuilder",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto registry = env->getNativeRegistry();
                require(registry->hasNativeFunction("__plugin_load"),
                        "EnvironmentBuilder should register __plugin_load");
                require(registry->hasNativeFunction("__plugin_unload"),
                        "EnvironmentBuilder should register __plugin_unload");
            });
    }

    void PluginTestSuite::setupNativesScriptTests()
    {
        /* Reserved for .mt-driven tests. setupIntegrationTests below covers
         * the OS-loader path via the plugin-test-fixture shared library. */
    }

    /* ============================================================
     * Integration test — real LoadLibrary/dlopen via fixture DLL
     * built by the `plugin-test-fixture` premake project. Lives at
     * mType/tests/testFiles/plugin/plugin_test_fixture.<ext>
     * ============================================================ */

    void PluginTestSuite::setupIntegrationTests()
    {
        namespace fs = std::filesystem;

        /* Resolve the fixture path. Premake's plugin-test-fixture project
         * outputs to mType/tests/testFiles/plugin/. Extension differs per OS;
         * try the platform-native one first, then the others (so a Windows
         * test binary handed an .so doesn't silently skip). */
        const std::string baseDir = "mType/tests/testFiles/plugin/";
        const std::string baseName = "plugin_test_fixture";

        std::vector<std::string> candidates;
#if defined(_WIN32)
        candidates.push_back(baseDir + baseName + ".dll");
#elif defined(__APPLE__)
        candidates.push_back(baseDir + baseName + ".dylib");
        candidates.push_back(baseDir + baseName + ".so");
#else
        candidates.push_back(baseDir + baseName + ".so");
#endif

        std::string fixturePath;
        for (const auto& c : candidates)
        {
            std::error_code ec;
            if (fs::exists(c, ec))
            {
                fixturePath = c;
                break;
            }
        }

        if (fixturePath.empty())
        {
            addSkippedTest(
                "Integration: load fixture DLL via PluginLoader",
                "plugin_test_fixture not found at " + baseDir
                + " (build the `plugin-test-fixture` project; CWD must be the project root)");
            addSkippedTest(
                "Integration: __pt_fixture_plus_seven returns arg + 7",
                "fixture DLL not present");
            addSkippedTest(
                "Integration: unload removes the registered native",
                "fixture DLL not present");
            return;
        }

        addCallbackTest("Integration: load fixture DLL via PluginLoader",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                require(::plugin::PluginLoader::instance().isLoaded(fixturePath),
                        "isLoaded should report true after load");
                require(env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "fixture should have registered __pt_fixture_plus_seven");

                /* Clean up so other tests get a fresh state. */
                ::plugin::PluginLoader::instance().unload(fixturePath, env, vm);
            });

        addCallbackTest("Integration: __pt_fixture_plus_seven returns arg + 7",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_fixture_plus_seven");
                require(static_cast<bool>(fn), "fixture native should be findable");

                ::value::Value out = invokeNative(fn, env, vm, { ::value::Value(int64_t{35}) });
                require(::value::isInt(out), "result should be int");
                require(::value::asInt(out) == 42,
                        "expected 42, got " + std::to_string(::value::asInt(out)));

                ::plugin::PluginLoader::instance().unload(fixturePath, env, vm);
            });

        addCallbackTest("Integration: unload removes the registered native",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                require(env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "precondition: fixture native registered");

                ::plugin::PluginLoader::instance().unload(fixturePath, env, vm);
                require(!env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "fixture native should be gone after unload");
                require(!::plugin::PluginLoader::instance().isLoaded(fixturePath),
                        "isLoaded should report false after unload");
            });
    }
}
