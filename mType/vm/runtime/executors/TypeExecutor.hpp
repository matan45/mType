#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
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
        TypeExecutor(ExecutionContext& ctx,
                     std::shared_ptr<environment::Environment> env);
        ~TypeExecutor() = default;

        // Type operations
        // MYT-320: INSTANCEOF entries inlined (trivial — delegate to static
        // checkInstanceOfByName). CAST entries kept out-of-line — they need
        // NativeArray + multi-dim array reconstruction and run cold (one
        // call per cast site, not per dispatch).
        inline void handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr) {
            const std::string& targetTypeName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            value::Value val = context.stackManager->pop();
            // Shared FFI entry point — same code path as ScriptAPI::isInstanceOf.
            bool result = checkInstanceOfByName(val, targetTypeName, environment);
            context.stackManager->push(result);
        }

        inline void handleInstanceofTypeParam(const bytecode::BytecodeProgram::Instruction& instr) {
            // The operand is a constant-pool string index holding the bare type
            // parameter name (e.g. "T"). Resolve it against the current receiver's
            // generic bindings, then chain into the existing instanceof machinery.
            const std::string& paramName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
            std::string resolved = resolveTypeParameter(paramName);
            value::Value val = context.stackManager->pop();
            bool result = checkInstanceOfByName(val, resolved, environment);
            context.stackManager->push(result);
        }

        void handleCast(const bytecode::BytecodeProgram::Instruction& instr);
        void handleCastTypeParam(const bytecode::BytecodeProgram::Instruction& instr);

        // MYT-228: stage method/free-function generic type-parameter
        // bindings into ExecutionContext::pendingTypeArgs. The very next
        // pushCallFrame consumes-and-clears that slot into the new
        // CallFrame::typeArgBindings.
        void handleBindTypeArgs(const bytecode::BytecodeProgram::Instruction& instr);

        // ============================================================
        // Public FFI entry point (MYT-42)
        //
        // Shared between the bytecode INSTANCEOF op and ScriptAPI so
        // language-level `obj isClassOf X` and native-level
        // ScriptAPI::isInstanceOf(obj, "X") are literally the same code
        // path. Pure static — uses `env` only to resolve interface
        // lookups during hierarchy walks.
        // ============================================================
        static bool checkInstanceOfByName(
            const value::Value& val,
            const std::string& targetTypeName,
            const std::shared_ptr<environment::Environment>& env);

        // Reconstruct the full parameterized type name for a runtime receiver.
        // For a new Box<Int>(...) instance returns "Box<Int>"; falls back to
        // the raw class name if the instance has no declared generic
        // parameters OR if its bindings are incomplete (partial / erased).
        // Matches the exact spacing produced by ast::GenericType::toString()
        // (", " after commas) so string equality against the compiler-emitted
        // target name is valid. Public for reuse by the FFI (MYT-42).
        // MYT-208: takes a raw ObjectInstance* so callers can pass either a
        // shared_ptr's .get() (OBJECT) or a borrowed STACK_OBJECT pointer
        // without re-wrapping. The function does not extend lifetime.
        static std::string reconstructObjectFullType(
            const runtimeTypes::klass::ObjectInstance* obj);

        // MYT-281: reconstruct the full type name for a multi-dim array Value
        // (FlatMultiArray / SparseMultiArray). Returns e.g. "int[][]" for a
        // rank-2 INT-defaulted bridge. Returns an empty string when the value
        // is not a multi-dim array, has no rank, or its element default tag
        // can't be classified — callers fall back to a permissive pass-through
        // in that case to preserve the prior behavior on inputs the primitive
        // multi-dim path doesn't currently produce.
        static std::string reconstructMultiArrayTypeName(const value::Value& val);

    private:
        ExecutionContext& context;
        std::shared_ptr<environment::Environment> environment;

        // Helper methods for type checking — static so they can run
        // without an ExecutionContext (reused by the FFI entry point).
        static bool checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName);
        static bool checkInstanceofObject(
            const value::Value& val,
            const std::string& targetTypeName,
            environment::Environment* env);

        // Resolve a type-parameter name (e.g. "T") to its bound concrete type
        // name via the current receiver's generic bindings. Throws a clean
        // RuntimeException if the parameter cannot be resolved (no this,
        // unbound parameter, etc.).
        std::string resolveTypeParameter(const std::string& paramName);

        static std::string reconstructValueObjectFullType(
            const std::shared_ptr<value::ValueObject>& obj);

        // Shared join helper used by both reconstruction paths.
        static std::string buildParameterizedName(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            const std::unordered_map<std::string, std::string>& bindings);

        // Helper methods for casting
        value::Value castToInt(const value::Value& val);
        value::Value castToFloat(const value::Value& val);
        value::Value castToString(const value::Value& val);
        value::Value castToBool(const value::Value& val);
        value::Value castToObject(const value::Value& val, const std::string& targetTypeName);

        // Interface hierarchy checking (raw mode — existing behavior)
        static bool checkInterfaceHierarchy(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited,
            environment::Environment* env
        );

        // Interface hierarchy checking (MYT-44 parameterized mode).
        // Takes a pre-substituted `interfaceName` string (e.g. "SortedList<Int>")
        // along with the bindings that produced it. At each recursion step the
        // bindings are rebuilt against the NEW interface's declared parameters
        // via rebindForInterface, so parameter names can differ across the chain.
        static bool checkInterfaceHierarchyParam(
            const std::string& interfaceName,
            const std::string& targetInterface,
            std::unordered_set<std::string>& visited,
            const std::unordered_map<std::string, std::string>& bindings,
            environment::Environment* env
        );

        // Convert value to string representation
        std::string valueToString(const value::Value& val);

        // Helper methods for castToObject
        struct TypeComponents {
            std::string baseName;
            std::string typeParams;
        };
        static TypeComponents extractTypeComponents(const std::string& typeName);
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
