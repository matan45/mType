#pragma once
#include <array>
#include <cstdint>
#include <deque>
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

namespace runtimeTypes::klass { class ClassDefinition; }
namespace runtimeTypes::global { class VariableDefinition; }

namespace vm::bytecode
{
    // MYT-197: 4-byte handle into a BytecodeProgram's frame-name interner.
    // Replaces std::string on hot-path carriers (CallFrame::functionName,
    // BytecodeLambda::functionName, Instruction::cachedMethodQualifiedName)
    // so per-call frame pushes do a 4-byte copy instead of a std::string
    // allocation. Always interned by the program that owns the frame — see
    // BytecodeProgram::internFrameName / getFrameName. Not serialized: the
    // interner is rebuilt lazily at runtime as push sites call internFrameName.
    struct FunctionNameHandle
    {
        uint32_t id;
        constexpr bool operator==(FunctionNameHandle other) const { return id == other.id; }
        constexpr bool operator!=(FunctionNameHandle other) const { return id != other.id; }
    };

    inline constexpr FunctionNameHandle INVALID_FN_HANDLE{ UINT32_MAX };

    /**
     * Represents a compiled bytecode program with constant pool and metadata
     * This is the output of the BytecodeCompiler and input to the VirtualMachine
     */
    class BytecodeProgram
    {
    public:
        // Forward declaration for inline cache
        struct FunctionMetadata;

        /**
         * Instruction structure: opcode + operands
         * PERFORMANCE: Includes inline cache for method calls
         */
        // Instruction flags for runtime optimization
        static constexpr uint8_t INSTR_FLAG_NONNULL_RECEIVER = 0x01;

        // MYT-201: all runtime-only IC / specialization / fusion state lives
        // in a sparse side table on BytecodeProgram, keyed by instruction
        // offset (see cachedStates below). Instruction itself is trimmed to
        // opcode + operands + flags so the interpreter hot path fits ~4x more
        // instructions per L1-D line.
        //
        // LIFETIME: cachedMethodShape / cachedFieldShape are raw pointers into
        // the class registry. The registry owns ClassDefinition objects for
        // the program's lifetime (registered at bytecode-load time and never
        // freed until shutdown), so these pointers remain stable across all
        // dispatches that could read them. They are cleared to nullptr on
        // deopt (see deoptAndReprocess / deoptGetFieldAndReprocess) — never
        // dangling, only null. Do NOT copy into any data structure with a
        // longer lifetime than a BytecodeProgram instance without revisiting
        // this.
        //
        // NEVER SERIALIZED: cachedStates is lazy runtime cache state, filled
        // by promote paths as opcodes specialize during execution. The .mtc
        // format rejects all CACHED/specialized opcodes on load (see
        // readInstructions), so a deserialized program always starts with an
        // empty side table and refills it through normal dispatch.
        struct CachedInstructionState
        {
            // CALL IC (generic function call — FunctionExecutor::handleCall).
            // Cached after first resolution; safe because the functions map
            // is immutable after compilation.
            const FunctionMetadata* cachedFuncMetadata = nullptr;
            size_t                  cachedStartOffset  = 0;
            const BytecodeProgram*  cachedProgram      = nullptr;
            size_t                  cachedProgramIndex = 0;

            // MYT-173: CALL_METHOD_CACHED embedded target. Once the method IC
            // at this IP stabilises to MONOMORPHIC, the opcode is rewritten
            // to CALL_METHOD_CACHED and these fields snapshot the IC entry,
            // so the hot path pays a single shape compare (no icTable probe,
            // no per-entry linear scan). On shape miss the opcode is rewritten
            // back and cachedMethodShape is cleared; cachedDeoptCount >= 1
            // makes the demote sticky.
            const runtimeTypes::klass::ClassDefinition* cachedMethodShape        = nullptr;
            const FunctionMetadata*                     cachedMethodFunc         = nullptr;
            const BytecodeProgram*                      cachedMethodProgram      = nullptr;
            size_t                                      cachedMethodProgramIndex = 0;
            // MYT-197: interned handle into cachedMethodProgram's frame-name
            // table. Snapshotted from MethodICEntry::qualifiedName at CACHED-
            // promote time; cleared to INVALID_FN_HANDLE in deoptAndReprocess.
            FunctionNameHandle                          cachedMethodQualifiedName{ INVALID_FN_HANDLE };
            // MYT-195: pre-resolved class prefix of cachedMethodQualifiedName.
            // Snapshotted from MethodICEntry::definingClassName at CACHED-
            // promote time so dispatchDirectFromCachedTarget doesn't re-split
            // the qualified name on every dispatch. Cleared in
            // deoptAndReprocess.
            std::string                                 cachedMethodDefiningClassName;
            // Shared sticky-demote gate: used by CALL_METHOD promotion AND by
            // LOAD_LOCAL / STORE_LOCAL type-quickening (MYT-199). Once >= 1,
            // the site permanently stays on the generic path.
            uint8_t                                     cachedDeoptCount         = 0;

            // MYT-203: CALL_METHOD_POLY_CACHED snapshot. When the IC at this
            // IP is POLYMORPHIC (entryCount in [2..4]), the opcode rewrites
            // to CALL_METHOD_POLY_CACHED and these arrays mirror
            // entries[0..polyEntryCount-1]. Re-snapshotted (idempotently) on
            // every promote call so a 3rd or 4th shape at an existing
            // POLY_CACHED site refreshes the array. polyEntryCount = 0 on
            // demote; arrays themselves are left stale (raw pointers + 4-byte
            // handle + std::string) since the demoted opcode never reads them
            // and the sticky polyCachedDeoptCount guarantees no re-promote.
            //
            // Independent sticky counter: cachedDeoptCount and
            // polyCachedDeoptCount deliberately do NOT alias. A site that
            // experienced a CACHED→CALL_METHOD deopt (cachedDeoptCount = 1)
            // can still try POLY_CACHED on its next stable phase. A site
            // that further deopts from POLY_CACHED stays generic forever via
            // polyCachedDeoptCount. Each tier has its own ping-pong defense.
            std::array<const runtimeTypes::klass::ClassDefinition*, 4> polyShapes{};
            std::array<const FunctionMetadata*,                     4> polyFuncs{};
            std::array<const BytecodeProgram*,                      4> polyPrograms{};
            std::array<size_t,                                      4> polyProgramIndices{};
            // polyQualifiedNames needs an explicit per-element initializer
            // because FunctionNameHandle's default-constructed value is 0,
            // which is a valid handle. The other arrays above can use {}
            // value-initialization safely: pointers default to nullptr and
            // size_t defaults to 0, both of which are sentinel for unset.
            std::array<FunctionNameHandle,                          4> polyQualifiedNames{
                { INVALID_FN_HANDLE, INVALID_FN_HANDLE, INVALID_FN_HANDLE, INVALID_FN_HANDLE }
            };
            std::array<std::string,                                 4> polyDefiningClassNames;
            uint8_t                                                    polyEntryCount = 0;
            uint8_t                                                    polyCachedDeoptCount = 0;

            // MYT-194: GET_FIELD_CACHED / SET_FIELD_CACHED embedded target.
            // Once the field IC at this IP stabilises to MONOMORPHIC, the
            // opcode is rewritten to the _CACHED variant and these fields
            // snapshot the resolved shape + field slot — the hot path becomes
            // a single shape compare + indexed field access. Kept parallel to
            // cachedMethod* (rather than shared) per ticket.
            const runtimeTypes::klass::ClassDefinition* cachedFieldShape     = nullptr;
            size_t                                      cachedFieldIndex     = static_cast<size_t>(-1);
            uint8_t                                     cachedFieldDeoptCount = 0;

            // MYT-204: LOAD_VAR_CACHED / STORE_VAR_CACHED embedded slot.
            // Snapshotted once the LOAD_VAR / STORE_VAR site at this IP
            // successfully resolves a global. Pointee is owned by
            // VariableManager's shared_ptr — heap-stable across map rehashes
            // and never removed at runtime (VariableManager::removeVariable
            // is only invoked from compile-time scope cleanup). No deopt
            // counter: globals are monomorphic by construction. The cached
            // executors guard against null on environment teardown and revert
            // to the generic path in that case.
            // Non-const pointer: STORE_VAR_CACHED needs to mutate via
            // setValue(); honestly modelling the mutability avoids a
            // const_cast on every store dispatch.
            runtimeTypes::global::VariableDefinition* cachedGlobalSlot = nullptr;

            // MYT-198: Superinstruction fusion. When a CACHED / ADD_INT
            // runtime rewrite fires, its predecessor LOAD_LOCAL / PUSH_INT
            // may be fused into the current instruction: the prior op becomes
            // NOP and this op is rewritten to a fused variant
            // (LOAD_LOCAL_CALL_CACHED, LOAD_LOCAL_GET_FIELD_CACHED, or
            // ADD_INT_CONST). fusedSlot carries the captured operand (local
            // slot for _CACHED fusions, int constant pool index for
            // ADD_INT_CONST). fusedDeoptCount makes the un-fuse decision
            // sticky — once >= 1, the same pair is never re-fused at this
            // site.
            uint32_t fusedSlot       = 0;
            uint8_t  fusedDeoptCount = 0;

            // MYT-199: per-site observed-type tag for LOAD_LOCAL / STORE_LOCAL
            // type-quickening. 0xFF is the "never observed" sentinel; any
            // other value is a value::ValueType cast to uint8_t, captured on
            // the first generic dispatch at this IP. The sticky-demote gate
            // reuses cachedDeoptCount above.
            uint8_t  observedValueType = 0xFF;
        };

        struct Instruction
        {
            OpCode opcode;
            std::vector<uint64_t> operands;
            uint8_t flags = 0; // Optimization flags (e.g., INSTR_FLAG_NONNULL_RECEIVER)

            Instruction();
            Instruction(OpCode op);
            Instruction(OpCode op, uint64_t operand1);
            Instruction(OpCode op, uint64_t operand1, uint64_t operand2);
            Instruction(OpCode op, std::vector<uint64_t> ops);
        };

        /**
         * Constant pool for storing literal values
         */
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

        /**
         * Global variable metadata for debugging
         */
        struct GlobalVariableMetadata
        {
            std::string name;
            std::string typeName; // Type as string (e.g., "Int", "String", "MyClass")
            value::ValueType type; // ValueType enum
            bool isFinal;
        };

        /// MYT-108: a single typed annotation argument (`key = literal`).
        /// Payload interpretation depends on `valueType`, matching
        /// ast::nodes::annotations::AnnotationValueType. Only the relevant
        /// payload field is meaningful for a given valueType.
        /// Moved above FunctionMetadata for MYT-110 — FunctionMetadata now
        /// carries AnnotationData vectors.
        struct TypedAnnotationArg
        {
            std::string key;
            uint8_t valueType;              // AnnotationValueType
            int64_t intVal = 0;             // INT
            double  floatVal = 0.0;         // FLOAT
            bool    boolVal = false;        // BOOL
            std::string stringVal;          // STRING / CLASS_REF
            std::vector<std::string> arrayVal; // CLASS_ARRAY
        };

        struct AnnotationData
        {
            std::string name;
            // Typed args survive full .mtc round-trip (MYT-108 v5+). Empty on
            // annotations that have no supplied parameters.
            std::vector<TypedAnnotationArg> typedArguments;
            errors::SourceLocation location; // Source location for error reporting
        };

        /**
         * Function metadata for compiled functions
         */
        struct FunctionMetadata
        {
            std::string name;
            std::string mangledName;
            size_t startOffset;
            size_t instructionCount;
            size_t localCount;
            size_t parameterCount;
            std::vector<std::string> parameterNames;
            std::vector<std::string> parameterTypes; // Added for type checking (without ? suffix)
            std::vector<bool> parameterNullable;    // Per-parameter nullable flags
            std::string returnType;
            bool isStatic = false;
            bool isNative = false;
            bool isAsync = false; // NEW: Flag for async functions
            std::vector<std::string> genericTypeParameters; // Generic type parameter names (e.g., ["T", "K", "V"])
            std::vector<std::string> localVariableNames; // NEW: Names of all local variables (for debugging)
            ExceptionTable exceptionTable; // Exception table for this function (try-catch-finally handlers)

            // MYT-110: function-level and per-parameter annotations (v7+).
            std::vector<AnnotationData> annotations;
            std::vector<std::vector<AnnotationData>> parameterAnnotations;

            // PHASE 2: Parameter patterns for nested generic type inference
            // Stores which type parameters are used in each parameter type
            // Example: for function<T, U> foo(Box<T> a, List<U> b)
            //   parameterTypeParameterUsage[0] = {"T"}  (Box<T> uses T)
            //   parameterTypeParameterUsage[1] = {"U"}  (List<U> uses U)
            std::vector<std::unordered_set<std::string>> parameterTypeParameterUsage;

            /**
             * Check if this function has nested type parameters in its parameter types
             * (i.e., type parameters that appear inside generic type arguments)
             */
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

        /**
         * Source location mapping for debugging
         */
        struct SourceLocation
        {
            uint32_t line;
            uint32_t column;
            std::string filename;
        };

        /// MYT-108: serialized annotation type declaration. Lives in the new
        /// top-level `annotationDeclarations` section of the .mtc file; on
        /// load, registered into Environment::annotationRegistry so that
        /// reflection getAnnotation() can resolve user-defined annotations.
        struct AnnotationParamSchemaData
        {
            std::string name;
            uint8_t declaredType;     // matches ast::nodes::annotations::AnnotationValueType
            bool nullable;
            bool isArray;
            bool hasDefault;
            // Default-value payload reuses the same valueType encoding as
            // declaredType, but is only present when hasDefault is true.
            int64_t defaultInt = 0;
            double  defaultFloat = 0.0;
            bool    defaultBool = false;
            std::string defaultString;          // STRING / CLASS_REF
            std::vector<std::string> defaultStringArray; // CLASS_ARRAY
        };

        struct AnnotationDeclData
        {
            std::string name;
            std::vector<AnnotationParamSchemaData> params;
            // MYT-109 (.mtc v6+): meta-annotations applied to this annotation
            // declaration, e.g. `@Retention(RUNTIME) @Target([METHOD])`.
            std::vector<AnnotationData> metaAnnotations;
        };

        /**
         * Class metadata for registration when loading cached bytecode
         */
        struct FieldMetadata
        {
            std::string name;
            std::string type;
            bool isStatic;
            bool isFinal;
            bool isPrivate;
            bool isProtected;
            // MYT-108: per-field annotations (string-pair fidelity; typed values
            // survive at runtime via the AnnotationRegistry schema lookup).
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
            bool isAbstract; // NEW: Abstract method flag
            size_t startOffset; // Where the method bytecode starts
            // MYT-108: per-method annotations
            std::vector<AnnotationData> annotations;
            // MYT-110: per-parameter annotations (v7+)
            std::vector<std::vector<AnnotationData>> parameterAnnotations;
        };

        struct ConstructorMetadata
        {
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            size_t startOffset; // Where the constructor bytecode starts
            // MYT-108: per-constructor annotations
            std::vector<AnnotationData> annotations;
            // MYT-110: per-parameter annotations (v7+)
            std::vector<std::vector<AnnotationData>> parameterAnnotations;
        };

        struct ClassMetadata
        {
            std::string name;
            std::string parentClassName;
            std::vector<std::string> implementedInterfaces;
            std::vector<std::string> genericParameters;
            bool isAbstract; // NEW: Abstract class flag
            bool isFinal; // Final class flag
            bool isValueClass; // NEW: Value class flag for value types
            std::vector<AnnotationData> annotations; // NEW: Class annotations (e.g., @Script)
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

        // MYT-197: frame-name interner. Disjoint from functionIndexToName above
        // (which backs CALL_FAST + .mtc serialization and must stay stable /
        // not see runtime-only names). A deque is used so existing string_view
        // keys in frameNameToId stay valid when new entries are pushed. Meta
        // is a parallel vector holding either &functions[name] (registered
        // function) or nullptr (runtime-only name like __script_main__,
        // interop-built ctor names, or the "<lambda>" display fallback).
        //
        // mutable so const BytecodeProgram* consumers (VM/executor convention
        // treats programs as read-mostly — see getMutableInstructionAt) can
        // intern on the hot path without casting at every call site. The
        // interner is effectively a runtime-filled cache, analogous to the
        // Instruction::cached* fields, which are also `mutable`.
        mutable std::deque<std::string> frameNameById;
        mutable std::unordered_map<std::string_view, uint32_t> frameNameToId;
        mutable std::vector<const FunctionMetadata*> frameNameToMeta;
        std::unordered_map<size_t, SourceLocation> sourceLocations;
        std::vector<ClassMetadata> classes; // Class metadata for cached bytecode
        std::vector<InterfaceMetadata> interfaces; // Interface metadata for cached bytecode
        std::vector<AnnotationDeclData> annotationDeclarations; // MYT-108 (.mtc v5+)
        std::vector<GlobalVariableMetadata> globalVariables; // Global variables for debugging
        ExceptionTable globalExceptionTable; // Exception table for global scope (try-catch-finally outside functions)
        size_t entryPoint;
        // Number of locals in the top-level "__script_main__" frame. Populated
        // at compile time and read by OSR to tier-up loops at script scope —
        // the top-level isn't registered as a FunctionMetadata, so without
        // this the runtime has no way to size the main frame. 0 when not set
        // (e.g. deserialized .mtc files pre-dating this field).
        size_t topLevelLocalCount = 0;
        std::string sourceFilePath; // For class registration when loading cached bytecode

        // MYT-198: lazy cache backing isFusionUnsafeTarget. `built` is the
        // dirty bit; cleared by any mutation path. The set itself is small in
        // practice (one entry per JUMP*, LOOP_START, function start, and
        // exception handler), so a hash set is fine vs a bitset.
        mutable std::unordered_set<size_t> fusionUnsafeTargets;
        mutable bool fusionUnsafeTargetsBuilt = false;
        void buildFusionUnsafeTargets() const;
        void invalidateFusionUnsafeTargets() const { fusionUnsafeTargetsBuilt = false; fusionUnsafeTargets.clear(); }

        // MYT-201: sparse side table for per-IP runtime cache state. Only
        // promoted / specialized / fused instructions ever carry an entry —
        // the <5% of sites that use it. Generic dispatch never touches this
        // map. `mutable` because executors access it through a
        // `const BytecodeProgram*` (same convention as the frame-name
        // interner and getMutableInstruction). Never serialized.
        mutable std::unordered_map<size_t, CachedInstructionState> cachedStates;

    public:
        BytecodeProgram();

        // Instruction Management
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

        // MYT-201: sparse cached-state accessors. getOrCreateCachedState
        // auto-inserts (use on write paths: promote / deopt / fuse).
        // findCachedState returns nullptr when no entry exists (use on
        // read paths: generic-path sticky-demote gate, plus hot CACHED
        // dispatch where an entry is invariant and can be asserted).
        CachedInstructionState& getOrCreateCachedState(size_t ip) const
        {
            return cachedStates[ip];
        }
        const CachedInstructionState* findCachedState(size_t ip) const
        {
            auto it = cachedStates.find(ip);
            return it == cachedStates.end() ? nullptr : &it->second;
        }

        // MYT-198: control-flow target query for runtime superinstruction fusion.
        // Returns true if `offset` is reachable from anywhere other than its
        // immediate predecessor — i.e. any JUMP* target, loop header, exception
        // handler entry, or function entry point. Fusion must NOT collapse a
        // pair whose second slot is such a target, because jumping directly to
        // the fused op would execute the prior (now-implicit) LOAD_LOCAL on a
        // stack that the jumping caller didn't set up for it.
        //
        // Lazily built on first call and memoised; invalidated by any call that
        // mutates the instruction vector (replaceInstructions / removeInstructions
        // / emit / updateAllJumpOffsets). Single-threaded VM, so no locking.
        bool isFusionUnsafeTarget(size_t offset) const;

        // Optimization Support (for peephole optimizer)
        void replaceInstructions(size_t offset, size_t count, const std::vector<Instruction>& newInstructions);
        void removeInstructions(size_t offset, size_t count);
        std::vector<Instruction> getInstructionRange(size_t start, size_t end) const;
        void updateAllJumpOffsets();  // Call after modifying instructions
        void updateFunctionOffsets(size_t removalOffset, int delta);  // Update function metadata after optimization
        void updateExceptionTableOffsets(size_t removalOffset, int delta);  // Update exception table offsets after optimization

        // Constant Pool Management
        ConstantPool& getConstantPool();
        const ConstantPool& getConstantPool() const;

        // Top-level script frame locals. See `topLevelLocalCount` above.
        void setTopLevelLocalCount(size_t count) { topLevelLocalCount = count; }
        size_t getTopLevelLocalCount() const { return topLevelLocalCount; }

        // Function Management
        void registerFunction(const std::string& name, const FunctionMetadata& metadata);
        const FunctionMetadata* getFunction(const std::string& name) const;
        const std::unordered_map<std::string, FunctionMetadata>& getFunctions() const;
        size_t getFunctionIndex(const std::string& name) const;
        const FunctionMetadata* getFunctionByIndex(size_t index) const;
        size_t getFunctionCount() const;

        // MYT-197: frame-name interning. internFrameName is idempotent; a
        // second call with the same text returns the same handle. registerFunction
        // calls this itself, so handles created before registration have their
        // metadata backfilled when the registration arrives. Callable on a
        // const* — the interner is mutable cache state.
        FunctionNameHandle internFrameName(std::string_view name) const;
        const std::string& getFrameName(FunctionNameHandle handle) const;
        const FunctionMetadata* getFunctionMeta(FunctionNameHandle handle) const;

        // Global Variable Management (for debugging)
        void registerGlobalVariable(const GlobalVariableMetadata& metadata);
        const std::vector<GlobalVariableMetadata>& getGlobalVariables() const;

        // Exception Table Management
        ExceptionTable& getGlobalExceptionTable();
        const ExceptionTable& getGlobalExceptionTable() const;

        // Source Location Management
        void addSourceLocation(size_t instructionOffset, uint32_t line, uint32_t column, const std::string& filename);
        const SourceLocation* getSourceLocation(size_t instructionOffset) const;

        // Entry Point
        void setEntryPoint(size_t offset);
        size_t getEntryPoint() const;

        // Source File Path (for class registration when loading cached bytecode)
        void setSourceFilePath(const std::string& path);
        const std::string& getSourceFilePath() const;

        // Class Metadata Management
        void registerClass(const ClassMetadata& classMeta);
        const std::vector<ClassMetadata>& getClasses() const;

        // Interface Metadata Management
        void addInterface(const InterfaceMetadata& interfaceMeta);
        const std::vector<InterfaceMetadata>& getInterfaces() const;

        // Annotation Declaration Metadata Management (MYT-108)
        void addAnnotationDeclaration(const AnnotationDeclData& declData);
        const std::vector<AnnotationDeclData>& getAnnotationDeclarations() const;

        // Async Detection
        bool hasAsyncFunctions() const;
        bool hasAwaitInstructions() const;

        // Disassembly and Serialization
        std::string disassemble() const;
        void serialize(std::ostream& out) const;
        static BytecodeProgram deserialize(std::istream& in);

    private:
        // Serialization helpers
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
        // MYT-108: reusable (de)serialization for per-target annotation lists.
        static void writeAnnotationList(std::ostream& out, const std::vector<AnnotationData>& list);
        static void readAnnotationList(std::istream& in, std::vector<AnnotationData>& list);

        // Source location update helper
        void updateSourceLocationsAfterOffset(size_t afterOffset, int delta);

        // Helper methods for writeClasses
        void writeFieldMetadata(std::ostream& out, const FieldMetadata& field) const;
        void writeMethodMetadata(std::ostream& out, const MethodMetadata& method) const;
        void writeConstructorMetadata(std::ostream& out, const ConstructorMetadata& ctor) const;
        void writeClassMetadata(std::ostream& out, const ClassMetadata& classMeta) const;

        // Helper methods for readClasses
        void readFieldMetadata(std::istream& in, FieldMetadata& field);
        void readMethodMetadata(std::istream& in, MethodMetadata& method);
        void readConstructorMetadata(std::istream& in, ConstructorMetadata& ctor);
        void readClassMetadata(std::istream& in, ClassMetadata& classMeta);

        // Helper methods for interfaces
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
