#include "ScriptApiNativeTestSuite.hpp"

#include "../../services/ScriptAPI.hpp"
#include "../../errors/ObjectException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/RuntimeException.hpp"

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

        // Bootstrap source shared by most tests. Seeds the interpreter
        // with:
        //   - Int primitive (via lib/primitives/Int.mt)
        //   - Box<T> generic wrapper (via lib/primitives/Box.mt)
        //   - Class reflection (via lib/reflect/Class.mt)
        //   - global `box` : Box<Int> — the parameterized subject
        //   - global `animal` : Animal — the non-generic subject
        //   - global `forNameHandle` : int — handle from the language-side
        //     Class.forName("Box<Int>") used for cross-surface identity
        //     checks
        const char* kBootstrap = R"MT(
import * from "mType/tests/testFiles/lib/reflect/Class.mt";
import * from "mType/tests/testFiles/lib/primitives/Int.mt";
import * from "mType/tests/testFiles/lib/primitives/Box.mt";

class Animal {
    public constructor() {}
}

Box<Int> box = new Box<Int>(42);
Animal animal = new Animal();
int forNameHandle = Class::forName("Box<Int>").getNativeHandle();
)MT";

        // Extract an int64 from a Class value's _nativeHandle field.
        // Proves identity checks without leaking internals to the test.
        int64_t handleOf(ScriptAPI& api, const Class& cls)
        {
            value::Value nameValue = api.callMethod(cls.asValue(), "getNativeHandle", {});
            if (!std::holds_alternative<int64_t>(nameValue))
            {
                throw std::runtime_error("Class.getNativeHandle() did not return an int");
            }
            return std::get<int64_t>(nameValue);
        }

        std::string classNameOf(ScriptAPI& api, const Class& cls)
        {
            value::Value nameValue = api.callMethod(cls.asValue(), "getName", {});
            if (std::holds_alternative<std::string>(nameValue))
            {
                return std::get<std::string>(nameValue);
            }
            if (std::holds_alternative<value::InternedString>(nameValue))
            {
                return std::get<value::InternedString>(nameValue).getString();
            }
            throw std::runtime_error("Class.getName() did not return a string");
        }
    }

    void ScriptApiNativeTestSuite::setupTests()
    {
        // ---- getGenericArguments ----

        addCallbackTest("getGenericArguments returns [Int] for Box<Int>",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.getVariable("box");
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
                value::Value animal = api.getVariable("animal");
                auto args = api.getGenericArguments(animal);
                require(args.empty(),
                    "expected empty type arguments, got " + std::to_string(args.size()));
            });

        // ---- isInstanceOf (string + Class overloads) ----

        addCallbackTest("isInstanceOf(box, \"Box<Int>\") == true",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.getVariable("box");
                require(api.isInstanceOf(box, "Box<Int>"),
                    "box should be instanceof Box<Int>");
            });

        addCallbackTest("isInstanceOf(box, classOf(\"Box<Int>\")) matches string overload",
            kBootstrap,
            [](ScriptAPI& api) {
                value::Value box = api.getVariable("box");
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
                value::Value box = api.getVariable("box");
                Class viaGetClass = api.getClass(box);
                Class viaClassOf = api.classOf("Box<Int>");
                require(handleOf(api, viaGetClass) == handleOf(api, viaClassOf),
                    "getClass(box) and classOf(\"Box<Int>\") must return the same interned handle");
            });

        addCallbackTest("classOf handle matches language-side Class.forName handle",
            kBootstrap,
            [](ScriptAPI& api) {
                // forNameHandle was captured inside the bootstrap via
                // language-side Class.forName("Box<Int>"). If this matches
                // the handle from native classOf, we've proven the single
                // ReflectionHandleRegistry is shared.
                value::Value langHandleVal = api.getVariable("forNameHandle");
                require(std::holds_alternative<int64_t>(langHandleVal),
                    "forNameHandle bootstrap variable must be an int");
                int64_t langHandle = std::get<int64_t>(langHandleVal);
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
                value::Value box = api.getVariable("box");
                auto args = api.getGenericArguments(box);
                require(args.size() == 1, "expected 1 type argument");

                // args[0].callMethod("getName") should work — proves the
                // returned Class is a live ObjectInstance, not a stub.
                value::Value name = api.callMethod(args[0].asValue(), "getName", {});
                std::string nameStr;
                if (std::holds_alternative<std::string>(name)) {
                    nameStr = std::get<std::string>(name);
                } else if (std::holds_alternative<value::InternedString>(name)) {
                    nameStr = std::get<value::InternedString>(name).getString();
                } else {
                    throw std::runtime_error("getName did not return a string");
                }
                require(nameStr == "Int",
                    "args[0].getName() should be Int, got " + nameStr);
            });
    }
}
