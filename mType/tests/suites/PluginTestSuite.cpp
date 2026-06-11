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
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../environment/registry/FieldDefinition.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../errors/RuntimeException.hpp"

#include "../../plugin/PluginHostApi.h"
#include "../../plugin/PluginHost.hpp"
#include "../../plugin/PluginLoader.hpp"
#include "../../plugin/PluginNatives.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
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

        /* RAII guard: ensures a loaded plugin is unloaded from the singleton
         * even if an assertion between load() and the natural unload() throws.
         * Without this, a failed assertion poisons every subsequent test on
         * the same path with "already loaded". */
        struct LoadedPluginGuard
        {
            std::string path;
            std::shared_ptr<::environment::Environment> env;
            std::shared_ptr<::vm::runtime::VirtualMachine> vm;
            ~LoadedPluginGuard()
            {
                try { ::plugin::PluginLoader::instance().unload(path, env, vm); }
                catch (...) {}
            }
        };

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

        addCallbackTest("arraySet out-of-bounds returns MT_ERR_RANGE",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                /* Plugin returns the status code arraySet returned for an OOB index. */
                auto probe = +[](void*, ::MTypeContext* ctx,
                                  const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* arr = host->makeArray(ctx, MT_TAG_INT, 2);
                    ::MTypeStatus st = host->arraySet(arr, 5, host->makeInt(ctx, 99));
                    return host->makeInt(ctx, static_cast<int64_t>(st));
                };
                auto binding = installSyntheticPlugin(env, "__pt_arr_oob", probe);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_arr_oob");
                ::value::Value out = invokeNative(fn, env, vm, {});
                require(::value::isInt(out), "result should be int");
                require(::value::asInt(out) == static_cast<int64_t>(MT_ERR_RANGE),
                        "OOB arraySet should return MT_ERR_RANGE, got "
                        + std::to_string(::value::asInt(out)));
            });

        addCallbackTest("makeObject + objSet + objGet round-trip an int field",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                /* Register a minimal class "__pt_obj { x: int }" so the host has
                 * something for makeObject(...) to resolve. */
                auto classDef = std::make_shared<::runtimeTypes::klass::ClassDefinition>("__pt_obj");
                auto xField = std::make_shared<::runtimeTypes::klass::FieldDefinition>(
                    "x", ::value::ValueType::INT);
                classDef->addInstanceField("x", xField);
                env->registerClass("__pt_obj", classDef);

                /* Plugin creates an instance, sets x, then reads x back and returns it. */
                auto rt = +[](void*, ::MTypeContext* ctx,
                              const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* obj = host->makeObject(ctx, "__pt_obj");
                    if (host->getTag(obj) != MT_TAG_OBJECT) {
                        host->raiseError(ctx, "TestError", "makeObject did not return OBJECT");
                        return host->makeNull(ctx);
                    }
                    host->objSet(obj, "x", host->makeInt(ctx, 73));
                    return host->objGet(ctx, obj, "x");
                };
                auto binding = installSyntheticPlugin(env, "__pt_obj_rt", rt);

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_obj_rt");
                ::value::Value out = invokeNative(fn, env, vm, {});
                require(::value::isInt(out), "result should be int after objGet");
                require(::value::asInt(out) == 73,
                        "expected 73, got " + std::to_string(::value::asInt(out)));
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

        /* Reentrancy: plugin native uses hostCallFunction to invoke an
         * mType function declared in reentrancy_bootstrap.mt. Exercises the
         * trampoline → vm->executeFunction → arenaPush return path as a unit
         * test (the example in hello.cpp does this end-to-end via a .mt
         * script). Single-level reentrancy only — chained executeFunction
         * composition is a separate VM concern. */
        addCallbackTest("hostCallFunction reentrantly invokes an mType function",
            "mType/tests/testFiles/plugin/reentrancy_bootstrap.mt",
            [](services::ScriptAPI& api) {
                auto env = api.getEnvironment();

                /* Plugin native: applyOnce(name, n) = double(n). */
                auto applyOnce = +[](void*, ::MTypeContext* ctx,
                                      const ::MTypeValue* const* args, int argc) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    if (argc != 2) {
                        host->raiseError(ctx, "TestError", "applyOnce: expected 2 args");
                        return host->makeNull(ctx);
                    }
                    size_t n = 0;
                    const char* funcName = host->getString(args[0], &n);
                    int64_t v = host->getInt(args[1]);

                    ::MTypeValue* a0 = host->makeInt(ctx, v);
                    const ::MTypeValue* call1[] = { a0 };
                    return host->callFunction(ctx, funcName, call1, 1);
                };
                auto binding = installSyntheticPlugin(env, "__pt_apply_once", applyOnce);

                /* Invoke via ScriptAPI so the route is identical to a real
                 * .mt script calling the plugin: env→native→trampoline→VM. */
                ::value::Value out = api.callFunction(
                    "__pt_apply_once",
                    { ::value::Value(std::string("double")), ::value::Value(int64_t{21}) });

                require(::value::isInt(out), "result should be int");
                require(::value::asInt(out) == 42,
                        "double(21) via callFunction should be 42, got "
                        + std::to_string(::value::asInt(out)));
            });

        addCallbackTest("hostCallFunction surfaces inner errors via errorPending",
            "mType/tests/testFiles/plugin/reentrancy_bootstrap.mt",
            [](services::ScriptAPI& api) {
                auto env = api.getEnvironment();

                /* Plugin calls a function that doesn't exist; we expect the
                 * trampoline to throw RuntimeException to the caller. */
                auto badCall = +[](void*, ::MTypeContext* ctx,
                                    const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    return host->callFunction(ctx, "__pt_no_such_fn_zz", nullptr, 0);
                };
                auto binding = installSyntheticPlugin(env, "__pt_bad_call", badCall);

                bool threw = false;
                try {
                    api.callFunction("__pt_bad_call", {});
                } catch (const ::errors::RuntimeException&) {
                    threw = true;
                }
                require(threw, "callFunction of unknown name should surface as an exception");
            });

        /* Deep reentrancy: the native calls the mType function bounce(d),
         * which calls the native again with d-1 — a plugin→mType→plugin
         * chain 6 frames deep. Each hop must save and restore the VM's
         * IP/call stack correctly or the count comes back wrong (or the VM
         * corrupts). The __native__ name prefix is what CompileTimeValidator
         * accepts as a runtime-resolved plugin native, letting the bootstrap
         * compile before the test registers the function.
         *
         * MYT-390: currently the chain recurses infinitely and kills the
         * process with 0xc00000fd (stack overflow) — single-level reentrancy
         * works, the defect starts at the second bytecode→native→bytecode
         * hop. A crash aborts the whole suite run, so the test is registered
         * as a SKIP until the fix lands; flip the flag below to re-arm it.
         * The body stays compiled so it cannot rot. */
        constexpr bool kMyt390DeepReentrancyFixed = false;
        if (!kMyt390DeepReentrancyFixed)
        {
            addSkippedTest("hostCallFunction nests 6 levels of plugin/mType reentrancy",
                "MYT-390: multi-hop plugin reentrancy stack-overflows the process "
                "(0xc00000fd); flip kMyt390DeepReentrancyFixed when the fix lands. "
                "Bootstrap: pluginProbe_bootstrap.mt");
        }
        else
        addCallbackTest("hostCallFunction nests 6 levels of plugin/mType reentrancy",
            "mType/tests/testFiles/plugin/pluginProbe_bootstrap.mt",
            [](services::ScriptAPI& api) {
                auto env = api.getEnvironment();

                auto deep = +[](void*, ::MTypeContext* ctx,
                                 const ::MTypeValue* const* args, int argc) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    if (argc != 1) {
                        host->raiseError(ctx, "TestError", "deep_recurse: expected 1 arg");
                        return host->makeNull(ctx);
                    }
                    int64_t d = host->getInt(args[0]);
                    if (d <= 0) {
                        return host->makeInt(ctx, 0);
                    }
                    ::MTypeValue* a0 = host->makeInt(ctx, d);
                    const ::MTypeValue* call1[] = { a0 };
                    return host->callFunction(ctx, "bounce", call1, 1);
                };
                auto binding = installSyntheticPlugin(env, "__native__pt_deep_recurse", deep);

                ::value::Value out = api.callFunction(
                    "__native__pt_deep_recurse", { ::value::Value(int64_t{6}) });
                require(::value::isInt(out), "deep reentrancy result should be int");
                require(::value::asInt(out) == 6,
                        "6-level reentrancy should count to 6, got "
                        + std::to_string(::value::asInt(out)));
            });

        /* callMethod reentrancy: only callFunction was covered. The plugin
         * constructs a ProbeBox and invokes its getX() method back in the VM.
         *
         * CANARY (MYT-390 family): currently returns a non-int — invokeMethod
         * from a plugin context appears to share the nested-execution defect
         * tracked in MYT-390 (multi-hop reentrancy). It fails WITHOUT
         * crashing, so unlike the deep-reentrancy skip above it stays armed
         * and failing until the fix lands
         * (memory: feedback_keep_failing_canary_tests). */
        addCallbackTest("CANARY hostCallMethod invokes a method on a constructed object",
            "mType/tests/testFiles/plugin/pluginProbe_bootstrap.mt",
            [](services::ScriptAPI& api) {
                auto env = api.getEnvironment();

                auto callGetX = +[](void*, ::MTypeContext* ctx,
                                     const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* obj = host->makeObject(ctx, "ProbeBox");
                    if (!obj || host->getTag(obj) != MT_TAG_OBJECT) {
                        host->raiseError(ctx, "TestError", "makeObject(ProbeBox) failed");
                        return host->makeNull(ctx);
                    }
                    return host->callMethod(ctx, obj, "getX", nullptr, 0);
                };
                auto binding = installSyntheticPlugin(env, "__pt_call_getx", callGetX);

                ::value::Value out = api.callFunction("__pt_call_getx", {});
                require(::value::isInt(out), "callMethod result should be int");
                require(::value::asInt(out) == 1,
                        "ProbeBox.getX() should return 1, got "
                        + std::to_string(::value::asInt(out)));
            });

        /* raiseError while an error is already pending: whichever message wins
         * (overwrite vs first-wins) the trampoline must surface ONE error to
         * the caller — never crash, never return a value as if nothing
         * happened. */
        addCallbackTest("raiseError while pending still surfaces exactly one error",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto doubleRaise = +[](void*, ::MTypeContext* ctx,
                                        const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    host->raiseError(ctx, "FirstError", "first message");
                    host->raiseError(ctx, "SecondError", "second message");
                    return host->makeNull(ctx);
                };
                auto binding = installSyntheticPlugin(env, "__pt_double_raise", doubleRaise);
                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_double_raise");

                bool threw = false;
                std::string msg;
                try {
                    invokeNative(fn, env, vm, {});
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "double raiseError must surface as an exception");
                require(msg.find("first message") != std::string::npos ||
                        msg.find("second message") != std::string::npos,
                        "surfaced error should carry one of the raised messages, got: " + msg);
            });

        /* arrayGet out of bounds: the bounds-checked getter must yield null
         * or a pending error — never a junk value pointer. */
        addCallbackTest("arrayGet out-of-bounds yields null or a pending error",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                auto oobProbe = +[](void*, ::MTypeContext* ctx,
                                     const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* arr = host->makeArray(ctx, MT_TAG_INT, 3);
                    ::MTypeValue* v = host->arrayGet(ctx, arr, 99);
                    /* hostArrayGet returns an arena-owned NULL-tagged value
                     * for OOB reads (a valid pointer, never nullptr) — both
                     * shapes count as the safe answer. */
                    int isNullish = (v == nullptr) ||
                                    (host->getTag(v) == MT_TAG_NULL);
                    return host->makeBool(ctx, isNullish);
                };
                auto binding = installSyntheticPlugin(env, "__pt_oob_get", oobProbe);
                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_oob_get");

                try {
                    ::value::Value out = invokeNative(fn, env, vm, {});
                    /* No pending error path: the probe must have observed null. */
                    require(::value::isBool(out) && ::value::asBool(out),
                            "arrayGet(99) on a 3-element array returned a non-null value");
                } catch (const ::errors::RuntimeException&) {
                    /* A raised pending error is the other acceptable contract. */
                }
            });

        /* objGet of a missing field: same null-or-pending-error contract. */
        addCallbackTest("objGet of a missing field yields null or a pending error",
            "mType/tests/testFiles/plugin/pluginProbe_bootstrap.mt",
            [](services::ScriptAPI& api) {
                auto env = api.getEnvironment();

                auto missingField = +[](void*, ::MTypeContext* ctx,
                                         const ::MTypeValue* const*, int) -> ::MTypeValue* {
                    const ::MTypePluginHost* host = ::plugin::getHostVTable();
                    ::MTypeValue* obj = host->makeObject(ctx, "ProbeBox");
                    if (!obj || host->getTag(obj) != MT_TAG_OBJECT) {
                        host->raiseError(ctx, "TestError", "makeObject(ProbeBox) failed");
                        return host->makeNull(ctx);
                    }
                    ::MTypeValue* v = host->objGet(ctx, obj, "noSuchField");
                    /* ObjectInstance::getFieldValue returns std::monostate{}
                     * (a VOID value) for a missing field, so the safe answers
                     * at the C ABI are: nullptr, a NULL-tagged value, or a
                     * VOID-tagged value. Anything else is a junk read. */
                    int isNullish = (v == nullptr) ||
                                    (host->getTag(v) == MT_TAG_NULL) ||
                                    (host->getTag(v) == MT_TAG_VOID);
                    return host->makeBool(ctx, isNullish);
                };
                auto binding = installSyntheticPlugin(env, "__pt_missing_field", missingField);

                try {
                    ::value::Value out = api.callFunction("__pt_missing_field", {});
                    require(::value::isBool(out) && ::value::asBool(out),
                            "objGet of a missing field returned a non-null value");
                } catch (const ::errors::RuntimeException&) {
                    /* pending-error contract also acceptable */
                }
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
                LoadedPluginGuard guard{fixturePath, env, vm};

                require(::plugin::PluginLoader::instance().isLoaded(fixturePath),
                        "isLoaded should report true after load");
                require(env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "fixture should have registered __pt_fixture_plus_seven");
            });

        addCallbackTest("Integration: __pt_fixture_plus_seven returns arg + 7",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                LoadedPluginGuard guard{fixturePath, env, vm};

                auto fn = env->getNativeRegistry()->findNativeFunction("__pt_fixture_plus_seven");
                require(static_cast<bool>(fn), "fixture native should be findable");

                ::value::Value out = invokeNative(fn, env, vm, { ::value::Value(int64_t{35}) });
                require(::value::isInt(out), "result should be int");
                require(::value::asInt(out) == 42,
                        "expected 42, got " + std::to_string(::value::asInt(out)));
            });

        addCallbackTest("Integration: unload removes the registered native",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                /* Guard catches everything, so the deliberate unload below
                 * leaves the guard's dtor with a harmless "not loaded" that
                 * gets swallowed — keeps cleanup correct if a require() fires. */
                LoadedPluginGuard guard{fixturePath, env, vm};
                require(env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "precondition: fixture native registered");

                ::plugin::PluginLoader::instance().unload(fixturePath, env, vm);
                require(!env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                        "fixture native should be gone after unload");
                require(!::plugin::PluginLoader::instance().isLoaded(fixturePath),
                        "isLoaded should report false after unload");
            });

        addCallbackTest("Integration: double-load of the same path throws",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                LoadedPluginGuard guard{fixturePath, env, vm};

                bool threw = false;
                std::string msg;
                try {
                    ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "second load() of an already-loaded path should throw");
                require(msg.find("already loaded") != std::string::npos,
                        "exception should mention 'already loaded', got: " + msg);
            });

        /* MYT-325 follow-up: load() must resolve a relative basename via a
         * registered search root. Mirrors the launcher's exeDir registration:
         * the launcher passes "mt_modules/.../foo.dll" verbatim, that path
         * doesn't exist as-is (CWD is `build/`), and resolution falls back to
         * <searchRoot>/mt_modules/.../foo.dll. */
        addCallbackTest("Integration: addSearchPath resolves a basename load",
            "",
            [fixturePath, baseName](services::ScriptAPI&) {
                namespace fs = std::filesystem;

                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                /* Use the fixture's parent dir as the search root, then load
                 * by just the leaf filename so the verbatim-exists check fails
                 * and resolution must consult the search root. */
                fs::path fixtureAbs = fs::weakly_canonical(fs::path(fixturePath));
                std::string searchRoot = fixtureAbs.parent_path().string();
                std::string leaf = fixtureAbs.filename().string();

                /* Sanity: the leaf must NOT exist relative to CWD, otherwise
                 * the verbatim-exists check would short-circuit the search
                 * path and the test wouldn't exercise the new behavior. */
                require(!fs::exists(leaf),
                    "Test setup invalid: CWD already contains '" + leaf +
                    "' so the search path won't be exercised");

                ::plugin::PluginLoader::instance().addSearchPath(searchRoot);

                /* load() should find the fixture via the search path. The
                 * loaded handle is keyed by the absolute resolved path, so
                 * unload() must use either the absolute path or the same
                 * relative path (the resolver handles both). */
                ::plugin::PluginLoader::instance().load(leaf, env, vm);
                LoadedPluginGuard guard{leaf, env, vm};

                require(::plugin::PluginLoader::instance().isLoaded(fixtureAbs.string()),
                    "isLoaded(absolute path) should report true after relative load");
                require(env->getNativeRegistry()->hasNativeFunction("__pt_fixture_plus_seven"),
                    "fixture native should be registered after resolved load");
            });

        /* MYT-325 follow-up: bogus path with no matching search root must
         * still fail with the standard "file not found" message, and must
         * surface the user-supplied path (not a search-expanded variant) so
         * the error is actionable. */
        addCallbackTest("Integration: load throws with original path when no search root matches",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                /* Add an unrelated search path — it must not mask the failure. */
                ::plugin::PluginLoader::instance().addSearchPath(
                    "mType/tests/testFiles/plugin");

                const std::string bogus = "some/nonexistent/__never_a_real_plugin_zz.dll";
                bool threw = false;
                std::string msg;
                try {
                    ::plugin::PluginLoader::instance().load(bogus, env, vm);
                } catch (const ::errors::RuntimeException& e) {
                    threw = true;
                    msg = e.what();
                }
                require(threw, "load of unresolvable path should throw");
                require(msg.find("not found") != std::string::npos,
                    "error should mention 'not found', got: " + msg);
                require(msg.find(bogus) != std::string::npos,
                    "error should echo the user-supplied path, got: " + msg);
            });

        /* ============================================================
         * Malformed library handling
         * ============================================================ */

        addCallbackTest("Integration: garbage bytes are rejected as a plugin",
            "",
            [](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                fs::path tmp = fs::temp_directory_path() / "_mtype_garbage_plugin.dll";
                {
                    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
                    f << "this is definitely not a shared library";
                }

                bool threw = false;
                try {
                    ::plugin::PluginLoader::instance().load(tmp.string(), env, vm);
                } catch (const std::exception&) {
                    threw = true;
                }
                const bool loaded = ::plugin::PluginLoader::instance().isLoaded(tmp.string());
                std::error_code ec;
                fs::remove(tmp, ec);

                require(threw, "garbage library must fail to load");
                require(!loaded, "garbage library must not be tracked as loaded");
            });

        /* Missing-entry fixture: a VALID shared library that simply lacks the
         * mtype_plugin_register export, built by plugin-test-fixture-noentry. */
        {
            const std::string noentryName = "plugin_test_fixture_noentry";
            std::vector<std::string> noentryCandidates;
#if defined(_WIN32)
            noentryCandidates.push_back(baseDir + noentryName + ".dll");
#elif defined(__APPLE__)
            noentryCandidates.push_back(baseDir + noentryName + ".dylib");
            noentryCandidates.push_back(baseDir + noentryName + ".so");
#else
            noentryCandidates.push_back(baseDir + noentryName + ".so");
#endif
            std::string noentryPath;
            for (const auto& c : noentryCandidates)
            {
                std::error_code ec;
                if (fs::exists(c, ec)) { noentryPath = c; break; }
            }

            if (noentryPath.empty())
            {
                addSkippedTest(
                    "Integration: library without mtype_plugin_register is rejected",
                    "plugin_test_fixture_noentry not found at " + baseDir
                    + " (build the `plugin-test-fixture-noentry` project)");
            }
            else
            {
                addCallbackTest("Integration: library without mtype_plugin_register is rejected",
                    "",
                    [noentryPath](services::ScriptAPI&) {
                        auto env = ::environment::EnvironmentBuilder().build();
                        auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                        bool threw = false;
                        try {
                            ::plugin::PluginLoader::instance().load(noentryPath, env, vm);
                        } catch (const std::exception&) {
                            threw = true;
                        }
                        const bool loaded =
                            ::plugin::PluginLoader::instance().isLoaded(noentryPath);

                        require(threw,
                            "library lacking the entry export must fail to load");
                        require(!loaded,
                            "entry-less library must not be tracked as loaded");
                    });
            }
        }

        /* ============================================================
         * Lifecycle churn and search-path edge cases
         * ============================================================ */

        addCallbackTest("Integration: 25 load/unload cycles stay clean",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                for (int i = 0; i < 25; ++i)
                {
                    ::plugin::PluginLoader::instance().load(fixturePath, env, vm);
                    require(env->getNativeRegistry()->hasNativeFunction(
                                "__pt_fixture_plus_seven"),
                            "cycle " + std::to_string(i) + ": native missing after load");
                    ::plugin::PluginLoader::instance().unload(fixturePath, env, vm);
                    require(!env->getNativeRegistry()->hasNativeFunction(
                                "__pt_fixture_plus_seven"),
                            "cycle " + std::to_string(i) + ": native lingered after unload");
                    require(!::plugin::PluginLoader::instance().isLoaded(fixturePath),
                            "cycle " + std::to_string(i) + ": still tracked after unload");
                }
            });

        addCallbackTest("Integration: duplicate search paths do not break basename loads",
            "",
            [fixturePath](services::ScriptAPI&) {
                auto env = ::environment::EnvironmentBuilder().build();
                auto vm = std::make_shared<::vm::runtime::VirtualMachine>(env);

                fs::path fixtureAbs = fs::weakly_canonical(fs::path(fixturePath));
                std::string searchRoot = fixtureAbs.parent_path().string();
                std::string leaf = fixtureAbs.filename().string();

                /* Registering the same root twice must be harmless. */
                ::plugin::PluginLoader::instance().addSearchPath(searchRoot);
                ::plugin::PluginLoader::instance().addSearchPath(searchRoot);

                ::plugin::PluginLoader::instance().load(leaf, env, vm);
                LoadedPluginGuard guard{leaf, env, vm};

                require(::plugin::PluginLoader::instance().isLoaded(fixtureAbs.string()),
                        "fixture should load once despite duplicate search roots");
                require(env->getNativeRegistry()->hasNativeFunction(
                            "__pt_fixture_plus_seven"),
                        "fixture native should be registered");
            });
    }
}
