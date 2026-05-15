#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <iosfwd>
#include "OpCode.hpp"
#include "ExceptionTable.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../value/ValueType.hpp"
#include "../../environment/registry/NativeDelegate.hpp"

namespace runtimeTypes::klass { class ClassDefinition; }
namespace runtimeTypes::global { class VariableDefinition; }

namespace vm::bytecode
{
    struct FunctionNameHandle
    {
        uint32_t id;
        constexpr bool operator==(FunctionNameHandle other) const { return id == other.id; }
        constexpr bool operator!=(FunctionNameHandle other) const { return id != other.id; }
    };

    inline constexpr FunctionNameHandle INVALID_FN_HANDLE{ UINT32_MAX };

    class BytecodeProgram
    {
    public:
        struct FunctionMetadata;

        static constexpr uint8_t INSTR_FLAG_NONNULL_RECEIVER = 0x01;

        struct CachedInstructionState
        {
            const FunctionMetadata*                     cachedFuncMetadata        = nullptr;
            size_t                                      cachedStartOffset         = 0;
            const BytecodeProgram*                      cachedProgram             = nullptr;
            size_t                                      cachedProgramIndex        = 0;
            ::environment::registry::NativeDelegate     cachedNativeFunc          = {};
            const runtimeTypes::klass::ClassDefinition* cachedMethodShape         = nullptr;
            const FunctionMetadata*                     cachedMethodFunc          = nullptr;
            const BytecodeProgram*                      cachedMethodProgram       = nullptr;
            size_t                                      cachedMethodProgramIndex  = 0;
            FunctionNameHandle                          cachedMethodQualifiedName { INVALID_FN_HANDLE };
            std::string                                 cachedMethodDefiningClassName;
            uint8_t                                     cachedDeoptCount          = 0;

            std::array<const runtimeTypes::klass::ClassDefinition*, 4> polyShapes{};
            std::array<const FunctionMetadata*,                     4> polyFuncs{};
            std::array<const BytecodeProgram*,                      4> polyPrograms{};
            std::array<size_t,                                      4> polyProgramIndices{};
            std::array<FunctionNameHandle,                          4> polyQualifiedNames{
                { INVALID_FN_HANDLE, INVALID_FN_HANDLE, INVALID_FN_HANDLE, INVALID_FN_HANDLE }
            };
            std::array<std::string,                                 4> polyDefiningClassNames;
            uint8_t                                                    polyEntryCount = 0;
            uint8_t                                                    polyCachedDeoptCount = 0;

            const runtimeTypes::klass::ClassDefinition* cachedFieldShape     = nullptr;
            size_t                                      cachedFieldIndex     = static_cast<size_t>(-1);
            uint8_t                                     cachedFieldDeoptCount = 0;

            runtimeTypes::global::VariableDefinition* cachedGlobalSlot = nullptr;

            uint32_t fusedSlot       = 0;
            uint8_t  fusedDeoptCount = 0;

            uint8_t  observedValueType = 0xFF;

            // BIND_TYPE_ARGS resolved-binding cache. Populated on first
            // execution when every binding kind is Concrete (the rawValue
            // string is the resolved type name itself, no per-call walk
            // needed). Subsequent calls bulk-copy these pairs into the
            // pending map and skip the per-binding constant-pool indexing
            // and Concrete/ForwardFromCaller branch. Empty by default;
            // ForwardFromCaller bindings are NOT cached (they depend on
            // the caller frame state, which changes per call).
            std::vector<std::pair<std::string, std::string>> cachedTypeArgBindings;
            bool cachedTypeArgBindingsValid = false;

            // jit_call_function_ic per-IP probe cache. The first call at this
            // IP probes jitCodeCache + nativeRegistry; subsequent calls reuse
            // the result. cachedJitFnPtr stores the JitFunction (or null when
            // the callee isn't JIT-compiled). cachedFrameName is pre-interned
            // for fast tryJitDispatchResolved use. jitProbeDone gates re-probe.
            //
            // Trade-off: a callee that gets JIT-compiled AFTER first probe at
            // this site won't be picked up. For hot loops (the only case that
            // matters for perf) the callee is either compiled before the loop
            // starts or never — re-polling per call would re-introduce the
            // hashmap probe cost the cache exists to eliminate.
            void* cachedJitFnPtr = nullptr;
            FunctionNameHandle cachedFrameName{ INVALID_FN_HANDLE };
            bool jitProbeDone = false;
        };

        // MYT-313: Inline-slot Instruction layout.
        //
        // Replaces the prior `std::vector<uint64_t> operands` (24-byte vector
        // header + per-instruction heap allocation for any non-empty operand
        // list) with three inline operand slots plus a heap overflow array
        // used only when the operand count exceeds 3.
        //
        // Operand storage rule (SPLIT layout):
        //   operands[0..min(2,count-1)] always live in inlineOperands[].
        //   operands[3..count-1] (when present) live in overflow[0..count-4].
        //
        // This means `inlineOperands[K]` for K in [0,2] is ALWAYS the correct
        // value regardless of total operand count — the hot-path callers
        // (LOAD_LOCAL, JUMP, PUSH_INT, ...) access operands [0..2] directly
        // with no branch. Variadic opcodes (LAMBDA, BIND_TYPE_ARGS,
        // NEW_OBJECT_WITH_FIELDS) use operandAt(K) for K >= 3, which dispatches
        // to overflow[K-3].
        //
        // Operand-count audit across the codebase: 99.2% of accesses target
        // operands[0..2] and become direct inline-array reads with zero
        // overhead vs. the old vector pointer-chase.
        struct Instruction
        {
            OpCode   opcode;                       // 1B
            uint8_t  flags = 0;                    // 1B
            uint8_t  operandCount = 0;             // 1B (total operands; 0..255)
            uint8_t  _reserved1 = 0;               // 1B padding
            uint32_t _reserved2 = 0;               // 4B padding (alignment)
            uint64_t inlineOperands[3] = {0, 0, 0};// 24B (operands[0..2])
            std::unique_ptr<uint64_t[]> overflow;  // 8B (operands[3..]; null iff count <= 3)
            // total: 40 bytes

            Instruction();
            Instruction(OpCode op);
            Instruction(OpCode op, uint64_t operand1);
            Instruction(OpCode op, uint64_t operand1, uint64_t operand2);
            Instruction(OpCode op, std::vector<uint64_t> ops);

            Instruction(const Instruction& other);
            Instruction(Instruction&& other) noexcept = default;
            Instruction& operator=(const Instruction& other);
            Instruction& operator=(Instruction&& other) noexcept = default;
            ~Instruction() = default;

            // === Read access ===
            bool hasOperands() const noexcept { return operandCount > 0; }
            size_t numOperands() const noexcept { return operandCount; }

            // Generic read; handles inline/overflow split.
            // For K in [0,2] callers should prefer `inlineOperands[K]` directly.
            uint64_t operandAt(size_t i) const noexcept {
                return i < 3 ? inlineOperands[i] : overflow[i - 3];
            }

            // === Mutation ===
            void clearOperands() noexcept {
                operandCount = 0;
                overflow.reset();
            }

            // Replace operands with a single value (fusion / IC promotion paths).
            void setSingleOperand(uint64_t v) noexcept {
                operandCount = 1;
                inlineOperands[0] = v;
                overflow.reset();
            }

            // Mutate operand[0] in place. Used by jump patching (target rewrite)
            // and peephole jump-threading. Operand 0 is always inline.
            void setOperand0(uint64_t v) noexcept {
                inlineOperands[0] = v;
            }

            // Load `count` operands from a contiguous source buffer (deserialize
            // path). Allocates overflow only if count > 3.
            void loadOperands(const uint64_t* src, size_t count);
        };

        struct ConstantPool
        {
            std::vector<int64_t> integers;
            std::vector<double> floats;
            std::vector<std::string> strings;
            std::unordered_map<std::string, size_t> stringIndexMap;

            size_t addInteger(int64_t value);
            size_t addFloat(double value);
            size_t addString(const std::string& value);
            int64_t getInteger(size_t index) const;
            double getFloat(size_t index) const;
            const std::string& getString(size_t index) const;
        };

        struct GlobalVariableMetadata
        {
            std::string name;
            std::string typeName;
            value::ValueType type;
            bool isFinal;
        };

        struct TypedAnnotationArg
        {
            std::string key;
            uint8_t valueType;
            int64_t intVal = 0;
            double  floatVal = 0.0;
            bool    boolVal = false;
            std::string stringVal;
            std::vector<std::string> arrayVal;
        };

        struct AnnotationData
        {
            std::string name;
            std::vector<TypedAnnotationArg> typedArguments;
            errors::SourceLocation location;
        };

        struct FunctionMetadata
        {
            std::string name;
            std::string mangledName;
            size_t startOffset;
            size_t instructionCount;
            size_t localCount;
            size_t parameterCount;
            std::vector<std::string> parameterNames;
            std::vector<std::string> parameterTypes;
            std::vector<bool> parameterNullable;
            std::string returnType;
            bool isStatic = false;
            bool isNative = false;
            bool isAsync = false;
            std::vector<std::string> genericTypeParameters;
            std::vector<std::string> localVariableNames;
            ExceptionTable exceptionTable;
            std::vector<AnnotationData> annotations;
            std::vector<std::vector<AnnotationData>> parameterAnnotations;
            std::vector<std::unordered_set<std::string>> parameterTypeParameterUsage;

            // Set at registration time when every parameterType is a primitive
            // (int, float, bool, string). Allows skipping the per-call
            // convertLambdaArgumentsToInterfaces probe on the hot path.
            bool allPrimitiveParams = false;

            bool hasNestedTypeParameters() const
            {
                for (const auto& usedParams : parameterTypeParameterUsage)
                {
                    if (!usedParams.empty())
                    {
                        return true;
                    }
                }
                return false;
            }
        };

        struct SourceLocation
        {
            uint32_t line;
            uint32_t column;
            std::string filename;
        };

        struct AnnotationParamSchemaData
        {
            std::string name;
            uint8_t declaredType;
            bool nullable;
            bool isArray;
            bool hasDefault;
            int64_t defaultInt = 0;
            double  defaultFloat = 0.0;
            bool    defaultBool = false;
            std::string defaultString;
            std::vector<std::string> defaultStringArray;
        };

        struct AnnotationDeclData
        {
            std::string name;
            std::vector<AnnotationParamSchemaData> params;
            std::vector<AnnotationData> metaAnnotations;
        };

        struct FieldMetadata
        {
            std::string name;
            std::string type;
            bool isStatic;
            bool isFinal;
            bool isPrivate;
            bool isProtected;
            std::vector<AnnotationData> annotations;
        };

        struct MethodMetadata
        {
            std::string name;
            std::string returnType;
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            bool isStatic;
            bool isFinal;
            bool isPrivate;
            bool isProtected;
            bool isAbstract;
            size_t startOffset;
            std::vector<AnnotationData> annotations;
            std::vector<std::vector<AnnotationData>> parameterAnnotations;
        };

        struct ConstructorMetadata
        {
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            size_t startOffset;
            std::vector<AnnotationData> annotations;
            std::vector<std::vector<AnnotationData>> parameterAnnotations;
        };

        struct ClassMetadata
        {
            std::string name;
            std::string parentClassName;
            std::vector<std::string> implementedInterfaces;
            std::vector<std::string> genericParameters;
            bool isAbstract;
            bool isFinal;
            bool isValueClass;
            std::vector<AnnotationData> annotations;
            std::vector<FieldMetadata> instanceFields;
            std::vector<FieldMetadata> staticFields;
            std::vector<MethodMetadata> instanceMethods;
            std::vector<MethodMetadata> staticMethods;
            std::vector<ConstructorMetadata> constructors;
        };

        struct InterfaceMethodSignature
        {
            std::string name;
            std::string returnType;
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            std::vector<std::string> genericTypeParameters;
        };

        struct InterfaceMetadata
        {
            std::string name;
            std::vector<std::string> genericParameters;
            std::vector<InterfaceMethodSignature> methods;
            std::vector<std::string> extendsInterfaces;
            bool isFinal = false;
        };

    private:
        std::vector<Instruction> instructions;
        ConstantPool constantPool;
        std::unordered_map<std::string, FunctionMetadata> functions;
        std::vector<std::string> functionIndexToName;
        std::unordered_map<std::string, size_t> functionNameToIndex;

        mutable std::deque<std::string> frameNameById;
        mutable std::unordered_map<std::string_view, uint32_t> frameNameToId;
        mutable std::vector<const FunctionMetadata*> frameNameToMeta;
        std::unordered_map<size_t, SourceLocation> sourceLocations;
        std::vector<ClassMetadata> classes;
        std::vector<InterfaceMetadata> interfaces;
        std::vector<AnnotationDeclData> annotationDeclarations;
        std::vector<std::string> staticInitializerFunctions;
        std::vector<GlobalVariableMetadata> globalVariables;
        ExceptionTable globalExceptionTable;
        size_t entryPoint;
        size_t topLevelLocalCount = 0;
        std::vector<std::string> topLevelLocalNames;
        std::string sourceFilePath;

        mutable std::unordered_set<size_t> fusionUnsafeTargets;
        mutable bool fusionUnsafeTargetsBuilt = false;
        void buildFusionUnsafeTargets() const;
        void invalidateFusionUnsafeTargets() const { fusionUnsafeTargetsBuilt = false; fusionUnsafeTargets.clear(); }

        mutable std::unordered_map<size_t, CachedInstructionState> cachedStates;

        // MYT-313: program-owned stable copies of operand slices for JIT helpers
        // that need to embed a raw operand-array pointer as an immediate
        // (STRUCT_HASH_INT / STRUCT_EQ_INT). Inline operands live inside the
        // Instruction object and would move if the std::vector<Instruction>
        // reallocates; this side-table holds heap-stable copies whose address
        // is valid for the program's lifetime.
        //
        // Using std::list (not std::vector) so element addresses remain stable
        // as new slices are appended — vector would invalidate previously
        // returned pointers on reallocation.
        mutable std::list<std::vector<uint64_t>> jitStableSlotPool;

    public:
        BytecodeProgram();

        // Materialize a stable, heap-owned copy of operand slice
        // [start, start+count) drawn from `instr` and return a pointer that
        // remains valid for the program's lifetime. JIT-compile call sites
        // embed this pointer as an immediate.
        const uint64_t* materializeStableOperandSlice(
            const Instruction& instr, size_t start, size_t count) const;

        // MYT-318: One-shot bytecode validator. Verifies that every
        // instruction in the stream carries at least the minimum number of
        // operands its opcode requires. Runs at program load (deserialize
        // tail / explicit caller) so executors on the hot path can drop their
        // defensive `if (numOperands() == 0) throw...` checks for any opcode
        // covered by the validator's operand-count table. Throws
        // std::runtime_error on violation; otherwise returns.
        void validateInstructionOperands() const;

        void emit(OpCode opcode);
        void emit(OpCode opcode, uint64_t operand);
        void emit(OpCode opcode, uint64_t operand1, uint64_t operand2);
        void emit(OpCode opcode, const std::vector<uint64_t>& operands);

        size_t getCurrentOffset() const;
        void patchJump(size_t instructionIndex, uint64_t targetOffset);
        void setLastInstructionFlags(uint8_t flags);

        const Instruction& getInstruction(size_t offset) const;
        Instruction& getMutableInstruction(size_t offset);
        const std::vector<Instruction>& getInstructions() const;
        size_t getInstructionCount() const;

        CachedInstructionState& getOrCreateCachedState(size_t ip) const
        {
            return cachedStates[ip];
        }
        const CachedInstructionState* findCachedState(size_t ip) const
        {
            auto it = cachedStates.find(ip);
            return it == cachedStates.end() ? nullptr : &it->second;
        }

        // Plugin unload hook: zero every populated cachedNativeFunc slot.
        // After a plugin is unloaded, any IC entry holding a NativeDelegate
        // whose `invoke` pointer lives in the unloaded image would jump into
        // freed code on next dispatch. This resets all FFI-cached slots and
        // forces the next call at each IP to re-resolve via NativeRegistry,
        // which will now miss for the unregistered names. Other cache fields
        // (cachedFuncMetadata, method-IC entries, etc.) are preserved.
        void clearNativeCacheSlots() const
        {
            for (auto& [ip, state] : cachedStates)
            {
                state.cachedNativeFunc = ::environment::registry::NativeDelegate{};
            }
        }

        bool isFusionUnsafeTarget(size_t offset) const;

        void replaceInstructions(size_t offset, size_t count, const std::vector<Instruction>& newInstructions);
        void removeInstructions(size_t offset, size_t count);
        std::vector<Instruction> getInstructionRange(size_t start, size_t end) const;
        void updateAllJumpOffsets();
        void updateFunctionOffsets(size_t removalOffset, int delta);
        void updateExceptionTableOffsets(size_t removalOffset, int delta);

        ConstantPool& getConstantPool();
        const ConstantPool& getConstantPool() const;

        void setTopLevelLocalCount(size_t count) { topLevelLocalCount = count; }
        size_t getTopLevelLocalCount() const { return topLevelLocalCount; }
        void setTopLevelLocalNames(const std::vector<std::string>& names) { topLevelLocalNames = names; }
        void setTopLevelLocalName(size_t slot, const std::string& name)
        {
            if (slot >= topLevelLocalNames.size())
            {
                topLevelLocalNames.resize(slot + 1);
            }
            topLevelLocalNames[slot] = name;
            if (slot + 1 > topLevelLocalCount)
            {
                topLevelLocalCount = slot + 1;
            }
        }
        const std::vector<std::string>& getTopLevelLocalNames() const { return topLevelLocalNames; }

        void registerFunction(const std::string& name, const FunctionMetadata& metadata);
        const FunctionMetadata* getFunction(const std::string& name) const;
        const std::unordered_map<std::string, FunctionMetadata>& getFunctions() const;
        size_t getFunctionIndex(const std::string& name) const;
        const FunctionMetadata* getFunctionByIndex(size_t index) const;
        size_t getFunctionCount() const;

        FunctionNameHandle internFrameName(std::string_view name) const;
        const std::string& getFrameName(FunctionNameHandle handle) const;
        const FunctionMetadata* getFunctionMeta(FunctionNameHandle handle) const;

        void registerGlobalVariable(const GlobalVariableMetadata& metadata);
        const std::vector<GlobalVariableMetadata>& getGlobalVariables() const;

        ExceptionTable& getGlobalExceptionTable();
        const ExceptionTable& getGlobalExceptionTable() const;

        void addSourceLocation(size_t instructionOffset, uint32_t line, uint32_t column, const std::string& filename);
        const SourceLocation* getSourceLocation(size_t instructionOffset) const;

        void setEntryPoint(size_t offset);
        size_t getEntryPoint() const;

        void setSourceFilePath(const std::string& path);
        const std::string& getSourceFilePath() const;

        void registerClass(const ClassMetadata& classMeta);
        const std::vector<ClassMetadata>& getClasses() const;

        void addInterface(const InterfaceMetadata& interfaceMeta);
        const std::vector<InterfaceMetadata>& getInterfaces() const;

        void addAnnotationDeclaration(const AnnotationDeclData& declData);
        const std::vector<AnnotationDeclData>& getAnnotationDeclarations() const;

        void addStaticInitializerFunction(const std::string& functionName);
        const std::vector<std::string>& getStaticInitializerFunctions() const;

        bool hasAsyncFunctions() const;
        bool hasAwaitInstructions() const;

        std::string disassemble() const;
        void serialize(std::ostream& out) const;
        static BytecodeProgram deserialize(std::istream& in);

    private:
        void writeConstantPool(std::ostream& out) const;
        void readConstantPool(std::istream& in);
        void writeInstructions(std::ostream& out) const;
        void readInstructions(std::istream& in);
        void writeFunctions(std::ostream& out) const;
        void readFunctions(std::istream& in);
        void writeSourceLocations(std::ostream& out) const;
        void readSourceLocations(std::istream& in);
        void writeClasses(std::ostream& out) const;
        void readClasses(std::istream& in);
        void writeInterfaces(std::ostream& out) const;
        void readInterfaces(std::istream& in);
        void writeGlobalExceptionTable(std::ostream& out) const;
        void readGlobalExceptionTable(std::istream& in);
        void writeAnnotationDeclarations(std::ostream& out) const;
        void readAnnotationDeclarations(std::istream& in);
        static void writeAnnotationList(std::ostream& out, const std::vector<AnnotationData>& list);
        static void readAnnotationList(std::istream& in, std::vector<AnnotationData>& list);

        void updateSourceLocationsAfterOffset(size_t afterOffset, int delta);

        void writeFieldMetadata(std::ostream& out, const FieldMetadata& field) const;
        void writeMethodMetadata(std::ostream& out, const MethodMetadata& method) const;
        void writeConstructorMetadata(std::ostream& out, const ConstructorMetadata& ctor) const;
        void writeClassMetadata(std::ostream& out, const ClassMetadata& classMeta) const;

        void readFieldMetadata(std::istream& in, FieldMetadata& field);
        void readMethodMetadata(std::istream& in, MethodMetadata& method);
        void readConstructorMetadata(std::istream& in, ConstructorMetadata& ctor);
        void readClassMetadata(std::istream& in, ClassMetadata& classMeta);

        void writeInterfaceMethodSignature(std::ostream& out, const InterfaceMethodSignature& sig) const;
        void readInterfaceMethodSignature(std::istream& in, InterfaceMethodSignature& sig);
        void writeInterfaceMetadata(std::ostream& out, const InterfaceMetadata& meta) const;
        void readInterfaceMetadata(std::istream& in, InterfaceMetadata& meta);
    };
}

namespace std
{
    template <>
    struct hash<vm::bytecode::FunctionNameHandle>
    {
        size_t operator()(vm::bytecode::FunctionNameHandle h) const noexcept
        {
            return hash<uint32_t>{}(h.id);
        }
    };
}
