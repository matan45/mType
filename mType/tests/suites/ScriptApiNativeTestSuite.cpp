#include "ScriptApiNativeTestSuite.hpp"
#include <cstdint>

#include "../../services/ScriptAPI.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/ObjectException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/RuntimeException.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;
    using services::api::Class;

    namespace
    {
        // Small helper — throws std::runtime_error with `msg` when `cond`
        // is false. The NATIVE_CALLBACK runner catches std::exception and
        // fails the test with the message.
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }

        // Committed bootstrap file shared by every native test — see
        // mType/tests/testFiles/scriptapi/bootstrap.mt for the source.
        // Running it as a real file (instead of a temp copy) lets its
        // relative imports resolve correctly.
        const std::string kBootstrap =
            "mType/tests/testFiles/scriptapi/bootstrap.mt";

        const std::string kStaticFinalInterop =
            "mType/tests/testFiles/scriptapi/staticFinalInterop.mt";

        // Extract an int64 from a Class value's _nativeHandle field.
        // Proves identity checks without leaking internals to the test.
        int64_t handleOf(ScriptAPI& api, const Class& cls)
        {
            value::Value nameValue = api.callMethod(cls.asValue(), "getNativeHandle", {});
            if (!value::isInt(nameValue))
            {
                throw std::runtime_error("Class.getNativeHandle() did not return an int");
            }
            return value::asInt(nameValue);
        }

        std::string classNameOf(ScriptAPI& api, const Class& cls)
        {
            value::Value nameValue = api.callMethod(cls.asValue(), "getName", {});
            if (value::isString(nameValue))
            {
                return value::asString(nameValue);
            }
            if (value::isInternedString(nameValue))
            {
                return value::asInternedString(nameValue).getString();
            }
            throw std::runtime_error("Class.getName() did not return a string");
        }

        void requireIntValue(const value::Value& value, int64_t expected, const std::string& label)
        {
            require(value::isInt(value), label + " must return an int");
            require(value::asInt(value) == expected,
                label + " expected " + std::to_string(expected) +
                ", got " + std::to_string(value::asInt(value)));
        }

        void requireFloatValue(const value::Value& value, double expected, const std::string& label)
        {
            require(value::isFloat(value), label + " must return a float");
            require(std::fabs(value::asFloat(value) - expected) < 0.000001,
                label + " returned unexpected float value");
        }

        void requireBoolValue(const value::Value& value, bool expected, const std::string& label)
        {
            require(value::isBool(value), label + " must return a bool");
            require(value::asBool(value) == expected, label + " returned unexpected bool value");
        }
    }

    void ScriptApiNativeTestSuite::setupTests()
    {
        // ---- MYT-370: static final initializers before ScriptAPI calls ----

        // MYT-370 (rebuild regression): re-running a build must re-initialize
        // static finals. The original MYT-370 fix made them resolve on the first
        // build; this guards the editor's Stop -> Build Scripts -> Play cycle.
        // The teardown (resetForRebuild) clears the class registry and calls
        // VirtualMachine::reset(); reloading re-registers fresh FieldDefinitions
        // (static ints back at their 0 default) under a NEW BytecodeProgram and
        // re-runs the static initializers. Before the fix, reset() left
        // executedStaticInitializers populated, so the new program's identically
        // named "<static_init>$static" was treated as already-run and skipped,
        // leaving the fresh RIGHT at 0 — the VK-1311 symptom where
        // Input::isMouseButtonDown(Mouse::RIGHT) read button 0. Not JIT-specific:
        // the dedup lives in the bytecode VM lifecycle and reproduces with JIT
        // on or off.
        addInterpreterCallbackTest("MYT-370 static finals re-initialize after rebuild",
            "",
            [](services::ScriptInterpreter& interp) {
                interp.parseAndRegisterClasses(kStaticFinalInterop);
                requireIntValue(interp.getStaticField("Myt370Constants", "RIGHT"), 1, "RIGHT (first build)");

                vm::bytecode::BytecodeProgram rebuilt = *interp.getVM()->getProgram();
                interp.resetForRebuild();
                interp.loadFromProgram(std::move(rebuilt), /*runStaticInitializers=*/true);

                requireIntValue(interp.getStaticField("Myt370Constants", "LEFT"), 0, "LEFT (after rebuild)");
                requireIntValue(interp.getStaticField("Myt370Constants", "RIGHT"), 1, "RIGHT (after rebuild)");
                requireIntValue(interp.getStaticField("Myt370Constants", "MIDDLE"), 2, "MIDDLE (after rebuild)");
            });

        addCallbackTest("MYT-370 getStaticField returns initialized static finals",
            kStaticFinalInterop,
            [](ScriptAPI& api) {
                requireIntValue(api.getStaticField("Myt370Constants", "LEFT"), 0, "LEFT");
                requireIntValue(api.getStaticField("Myt370Constants", "RIGHT"), 1, "RIGHT");
                requireIntValue(api.getStaticField("Myt370Constants", "MIDDLE"), 2, "MIDDLE");
                requireFloatValue(api.getStaticField("Myt370Constants", "SCALE"), 1.5, "SCALE");
                requireBoolValue(api.getStaticField("Myt370Constants", "ENABLED"), true, "ENABLED");

                value::Value point = api.getStaticField("Myt370Constants", "POINT");
                require(value::isObject(point), "POINT must be an object");
                requireIntValue(api.callMethod(point, "sum", {}), 7, "POINT.sum()");

                value::Value values = api.getStaticField("Myt370Constants", "VALUES");
                require(value::isNativeArray(values), "VALUES must be a native array");
                const auto& array = value::asNativeArray(values);
                require(array->size() == 3, "VALUES length must be 3");
                requireIntValue(array->get(0), 8, "VALUES[0]");
                requireIntValue(array->get(1), 13, "VALUES[1]");
                requireIntValue(array->get(2), 21, "VALUES[2]");
            });

        addCallbackTest("MYT-370 Class::FIELD works in static methods after interop load",
            kStaticFinalInterop,
            [](ScriptAPI& api) {
                requireIntValue(api.callStaticMethod("Myt370Constants", "staticRightViaClassAccess", {}),
                    1, "staticRightViaClassAccess()");
                requireFloatValue(api.callStaticMethod("Myt370Constants", "staticScaleViaClassAccess", {}),
                    1.5, "staticScaleViaClassAccess()");
                requireBoolValue(api.callStaticMethod("Myt370Constants", "staticEnabledViaClassAccess", {}),
                    true, "staticEnabledViaClassAccess()");
                requireIntValue(api.callStaticMethod("Myt370Constants", "staticPointSumViaClassAccess", {}),
                    7, "staticPointSumViaClassAccess()");
                requireIntValue(api.callStaticMethod("Myt370Constants", "staticArrayMiddleViaClassAccess", {}),
                    13, "staticArrayMiddleViaClassAccess()");
            });

        addCallbackTest("MYT-370 Class::FIELD works in object methods after interop load",
            kStaticFinalInterop,
            [](ScriptAPI& api) {
                value::Value reader = api.createObject("Myt370Reader", {});
                requireIntValue(api.callMethod(reader, "readRight", {}), 1, "readRight()");
                requireFloatValue(api.callMethod(reader, "readScale", {}), 1.5, "readScale()");
                requireBoolValue(api.callMethod(reader, "readEnabled", {}), true, "readEnabled()");
                requireIntValue(api.callMethod(reader, "readPointSum", {}), 7, "readPointSum()");
                requireIntValue(api.callMethod(reader, "readArrayLast", {}), 21, "readArrayLast()");
            });

        // ---- getGenericArguments ----

        addCallbackTest("getGenericArguments returns [Int] for Box<Int>",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.callFunction("makeBox", {});
                auto args = api.getGenericArguments(box);
                require(args.size() == 1,
                    "expected 1 type argument, got " + std::to_string(args.size()));
                require(classNameOf(api, args[0]) == "Int",
                    "expected first type argument to be Int, got " +
                    classNameOf(api, args[0]));
            });

        addCallbackTest("getGenericArguments returns empty for non-generic",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value animal = api.callFunction("makeAnimal", {});
                auto args = api.getGenericArguments(animal);
                require(args.empty(),
                    "expected empty type arguments, got " + std::to_string(args.size()));
            });

        // ---- isInstanceOf (string + Class overloads) ----

        addCallbackTest("isInstanceOf(box, \"Box<Int>\") == true",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.callFunction("makeBox", {});
                require(api.isInstanceOf(box, "Box<Int>"),
                    "box should be instanceof Box<Int>");
            });

        addCallbackTest("isInstanceOf(box, classOf(\"Box<Int>\")) matches string overload",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.callFunction("makeBox", {});
                Class cls = api.classOf("Box<Int>");
                require(api.isInstanceOf(box, cls),
                    "Class-overload should agree with string overload");
                require(api.isInstanceOf(box, "Box<Int>") == api.isInstanceOf(box, cls),
                    "string and Class overloads must return identical results");
            });

        // ---- getClass cross-surface identity ----

        addCallbackTest("getClass(box) identity matches Class.forName(\"Box<Int>\")",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.callFunction("makeBox", {});
                Class viaGetClass = api.getClass(box);
                Class viaClassOf = api.classOf("Box<Int>");
                require(handleOf(api, viaGetClass) == handleOf(api, viaClassOf),
                    "getClass(box) and classOf(\"Box<Int>\") must return the same interned handle");
            });

        addCallbackTest("classOf handle matches language-side Class.forName handle",
            kBootstrap,
            [](ScriptAPI& api) {
                // langForNameHandle() captures the native handle from the
                // language-side Class.forName("Box<Int>"). If the native
                // classOf handle matches, the same ReflectionHandleRegistry
                // is shared across both surfaces.
                value::Value langHandleVal = api.callFunction("langForNameHandle", {});
                require(value::isInt(langHandleVal),
                    "langForNameHandle() must return an int");
                int64_t langHandle = value::asInt(langHandleVal);
                Class nativeCls = api.classOf("Box<Int>");
                require(handleOf(api, nativeCls) == langHandle,
                    "native classOf and language forName must converge on the same handle");
            });

        // ---- Error paths ----

        addCallbackTest("classOf(\"NonExistentType\") throws cleanly",
            kBootstrap,
            [](ScriptAPI& api) {
                bool threw = false;
                try {
                    api.classOf("NonExistentTypeXYZ");
                } catch (const std::exception&) {
                    threw = true;
                }
                require(threw, "classOf on unknown type must throw");
            });

        addCallbackTest("getClass on non-object throws cleanly",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value notAnObject(int64_t{1});
                bool threw = false;
                try {
                    api.getClass(notAnObject);
                } catch (const errors::ObjectException&) {
                    threw = true;
                }
                require(threw, "getClass on int value must throw ObjectException");
            });

        addCallbackTest("getGenericArguments on non-object throws cleanly",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value notAnObject(std::string("hello"));
                bool threw = false;
                try {
                    api.getGenericArguments(notAnObject);
                } catch (const errors::ObjectException&) {
                    threw = true;
                }
                require(threw, "getGenericArguments on string must throw ObjectException");
            });

        // ---- Smoke: returned Class is callable through ScriptAPI ----

        addCallbackTest("returned Class is callable via ScriptAPI::callMethod",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.callFunction("makeBox", {});
                auto args = api.getGenericArguments(box);
                require(args.size() == 1, "expected 1 type argument");

                // args[0].callMethod("getName") should work — proves the
                // returned Class is a live ObjectInstance, not a stub.
                value::Value name = api.callMethod(args[0].asValue(), "getName", {});
                std::string nameStr;
                if (value::isString(name)) {
                    nameStr = value::asString(name);
                } else if (value::isInternedString(name)) {
                    nameStr = value::asInternedString(name).getString();
                } else {
                    throw std::runtime_error("getName did not return a string");
                }
                require(nameStr == "Int",
                    "args[0].getName() should be Int, got " + nameStr);
            });
    }
}
