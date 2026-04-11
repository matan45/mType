#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include <functional>
#include <unordered_set>

namespace vm::runtime
{
    /**
     * Executes type-related opcodes
     * Handles INSTANCEOF, CAST
     * Performs runtime type checking, casting, and inheritance validation
     */
    class TypeExecutor
    {
    public:
        explicit TypeExecutor(ExecutionContext& ctx);
        ~TypeExecutor() = default;

        // Type operations
        void handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInstanceofTypeParam(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCast(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods for type checking
        bool checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName);
        bool checkInstanceofObject(const value::Value& val, const std::string& targetTypeName);

        // Resolve a type-parameter name (e.g. "T") to its bound concrete type
        // name via the current receiver's generic bindings. Throws a clean
        // RuntimeException if the parameter cannot be resolved (no this,
        // unbound parameter, etc.).
        std::string resolveTypeParameter(const std::string& paramName);

        // Reconstruct the full parameterized type name for a runtime receiver.
        // For a new Box<Int>(...) instance returns "Box<Int>"; falls back to
        // the raw class name if the instance has no declared generic
        // parameters OR if its bindings are incomplete (partial / erased).
        // Matches the exact spacing produced by ast::GenericType::toString()
        // (", " after commas) so string equality against the compiler-emitted
        // target name is valid.
        std::string reconstructObjectFullType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);
        std::string reconstructValueObjectFullType(
            const std::shared_ptr<value::ValueObject>& obj);

        // Shared join helper used by both reconstruction paths.
        std::string buildParameterizedName(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            const std::unordered_map<std::string, std::string>& bindings);

        // Helper methods for casting
        value::Value castToInt(const value::Value& val);
        value::Value castToFloat(const value::Value& val);
        value::Value castToString(const value::Value& val);
        value::Value castToBool(const value::Value& val);
        value::Value castToObject(const value::Value& val, const std::string& targetTypeName);

        // Interface hierarchy checking (raw mode — existing behavior)
        bool checkInterfaceHierarchy(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited
        );

        // Interface hierarchy checking (MYT-44 parameterized mode).
        // Takes a pre-substituted `interfaceName` string (e.g. "SortedList<Int>")
        // along with the bindings that produced it. At each recursion step the
        // bindings are rebuilt against the NEW interface's declared parameters
        // via rebindForInterface, so parameter names can differ across the chain.
        bool checkInterfaceHierarchyParam(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited,
            const std::unordered_map<std::string, std::string>& bindings
        );

        // Convert value to string representation
        std::string valueToString(const value::Value& val);

        // Helper methods for castToObject
        struct TypeComponents {
            std::string baseName;
            std::string typeParams;
        };
        TypeComponents extractTypeComponents(const std::string& typeName);
        bool checkExactMatch(const std::string& className, const std::string& targetTypeName,
                            const TypeComponents& classComp, const TypeComponents& targetComp);
        bool checkUpcastMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                             const std::string& targetTypeName, const TypeComponents& targetComp,
                             const std::string& baseClassName, const std::string& classTypeParams);
        bool checkDowncastMatch(const std::string& baseClassName, const std::string& baseTargetName,
                               const std::string& classTypeParams, const std::string& targetTypeParams);
        bool checkInterfaceMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                const std::string& targetTypeName);
        void throwIncompatibleCastError(const std::string& className, const std::string& targetTypeName);
        [[noreturn]] void throwCastError(const std::string& message);
    };
}
