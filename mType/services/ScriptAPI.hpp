#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include "api/Class.hpp"

namespace vm::runtime
{
    class VirtualMachine;
}

namespace vm::bytecode
{
    class BytecodeProgram;
}

namespace services
{
    /**
     * Service providing C++ to mType calling interface
     * Allows C++ code to call mType functions, methods, and manipulate objects
     *
     * FFI contract: native authors must NOT unwrap
     * `std::shared_ptr<runtimeTypes::klass::ObjectInstance>` out of a
     * `value::Value` in user code. Use the typed accessors on this class
     * (`getClass`, `getGenericArguments`, `isInstanceOf`, `classOf`,
     * `getObjectClassName`, `getField`, `setField`, etc.) which centralize
     * the variant check and route through the same reflection path as the
     * language surface.
     *
     * Hygiene grep (should return zero matches under services/):
     *   grep -rn "std::get<std::shared_ptr<.*ObjectInstance>>" mType/services/
     */
    class ScriptAPI
    {
    private:
        std::shared_ptr<environment::Environment> environment;

        // Bytecode VM support
        vm::runtime::VirtualMachine* vm;
        const vm::bytecode::BytecodeProgram* program;

    public:
        ScriptAPI(std::shared_ptr<environment::Environment> env,
                 vm::runtime::VirtualMachine* virtualMachine,
                 const vm::bytecode::BytecodeProgram* bytecodeProgram = nullptr);
        ~ScriptAPI();

        // Update bytecode program reference (for bytecode mode)
        void setBytecodeProgram(const vm::bytecode::BytecodeProgram* bytecodeProgram);

        // Function calling
        value::Value callFunction(const std::string& functionName,
                                 const std::vector<value::Value>& args = {});

        // Method calling on objects
        value::Value callMethod(const value::Value& object,
                               const std::string& methodName,
                               const std::vector<value::Value>& args = {});

        // Static method calling
        value::Value callStaticMethod(const std::string& className,
                                     const std::string& methodName,
                                     const std::vector<value::Value>& args = {});

        // Lambda invocation from C++
        value::Value callLambda(const value::Value& lambda,
                               const std::vector<value::Value>& args = {});

        // Instance field access
        value::Value getField(const value::Value& object, const std::string& fieldName);
        void setField(const value::Value& object,
                     const std::string& fieldName,
                     const value::Value& value);

        // Static field access
        value::Value getStaticField(const std::string& className, const std::string& fieldName);
        void setStaticField(const std::string& className,
                           const std::string& fieldName,
                           const value::Value& value);

        // Variable access
        value::Value getVariable(const std::string& variableName);
        void setVariable(const std::string& variableName, const value::Value& value);

        // Object creation and manipulation
        value::Value createObject(const std::string& className,
                                 const std::vector<value::Value>& constructorArgs = {});

        // Utility methods for working with objects
        bool isObjectOfClass(const value::Value& object, const std::string& className);
        std::string getObjectClassName(const value::Value& object);

        // ================================================================
        // Generic type metadata / Class FFI (MYT-42)
        //
        // All methods below route through the same reflection path as the
        // language surface (Class.forName / Class.getTypeArguments), so
        // identity is preserved across the FFI boundary — the Class
        // returned by classOf("Box<Int>") is the same interned instance
        // as language-side Class.forName("Box<Int>").
        // ================================================================

        /**
         * Get the runtime Class of an object, including parameterized type
         * arguments for generic instances. Equivalent to language-side
         * obj.getClass().
         *
         * Example:
         *   Value box = api.createObject("Box", { Value(int64_t{42}) });
         *   api::Class cls = api.getClass(box);   // Class for "Box<Int>"
         *
         * @throws ObjectException if `object` is not an ObjectInstance.
         */
        api::Class getClass(const value::Value& object);

        /**
         * Look up a Class by its canonical name. Accepts both concrete
         * names ("String", "Int") and parameterized forms ("Box<Int>",
         * "Map<String, List<Int>>"). Equivalent to language-side
         * Class.forName(...).
         *
         * @throws ClassNotFoundException if `typeName` does not resolve.
         */
        api::Class classOf(const std::string& typeName);

        /**
         * Get the runtime generic type arguments of an object as Class
         * values. Order matches parameter declaration order. Empty vector
         * for non-generic instances.
         *
         * Example:
         *   auto args = api.getGenericArguments(box);  // [Class for "Int"]
         *
         * @throws ObjectException if `object` is not an ObjectInstance.
         */
        std::vector<api::Class> getGenericArguments(const value::Value& object);

        /**
         * String-typed parameterized instance-of check. Mirrors
         * language-level `obj isClassOf X` including parameterized type
         * matching. For non-object values returns false.
         */
        bool isInstanceOf(const value::Value& object,
                          const std::string& fullyParameterizedType);

        /**
         * Class-typed instance-of check. Preferred for hot paths —
         * avoids a second name lookup. Identical semantics to the string
         * overload.
         */
        bool isInstanceOf(const value::Value& object, const api::Class& cls);
    };
}
