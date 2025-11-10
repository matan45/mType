#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
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
        void handleCast(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods for type checking
        bool checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName);
        bool checkInstanceofObject(const value::Value& val, const std::string& targetTypeName);

        // Helper methods for casting
        value::Value castToInt(const value::Value& val);
        value::Value castToFloat(const value::Value& val);
        value::Value castToString(const value::Value& val);
        value::Value castToBool(const value::Value& val);
        value::Value castToObject(const value::Value& val, const std::string& targetTypeName);

        // Interface hierarchy checking
        bool checkInterfaceHierarchy(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited
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
