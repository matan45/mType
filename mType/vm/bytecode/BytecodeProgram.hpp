#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iosfwd>
#include "OpCode.hpp"
#include "ExceptionTable.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../value/ValueType.hpp"

namespace runtimeTypes::klass { class ClassDefinition; }

namespace vm::bytecode
{
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

        struct Instruction
        {
            OpCode opcode;
            std::vector<uint64_t> operands;
            uint8_t flags = 0; // Optimization flags (e.g., INSTR_FLAG_NONNULL_RECEIVER)

            // PERFORMANCE: Inline cache for method calls - avoids repeated hash lookups
            // Cached after first resolution, used on subsequent calls.
            // Safe because the functions map is immutable after compilation.
            mutable const FunctionMetadata* cachedFuncMetadata = nullptr;
            mutable size_t cachedStartOffset = 0;
            mutable const BytecodeProgram* cachedProgram = nullptr;
            mutable size_t cachedProgramIndex = 0;

            // MYT-173: CALL_METHOD_CACHED embedded target. Once the method IC at
            // this IP has stabilized to MONOMORPHIC, the opcode is rewritten to
            // CALL_METHOD_CACHED and these fields snapshot the IC entry, so the
            // hot path pays a single shape compare (no icTable hashmap probe, no
            // per-entry linear scan). On shape miss the opcode is rewritten back
            // to CALL_METHOD and cachedMethodShape is cleared; cachedDeoptCount>=1
            // makes the demote sticky so we don't flip/unflip on unstable sites.
            // These fields are disjoint from cachedFuncMetadata et al., which serve
            // the CALL opcode (see FunctionExecutor).
            mutable const runtimeTypes::klass::ClassDefinition* cachedMethodShape = nullptr;
            mutable const FunctionMetadata* cachedMethodFunc = nullptr;
            mutable const BytecodeProgram* cachedMethodProgram = nullptr;
            mutable size_t cachedMethodProgramIndex = 0;
            mutable std::string cachedMethodQualifiedName;
            mutable uint8_t cachedDeoptCount = 0;

            // MYT-194: GET_FIELD_CACHED / SET_FIELD_CACHED embedded target.
            // Once the field IC at this IP has stabilized to MONOMORPHIC, the
            // opcode is rewritten to the _CACHED variant and these fields
            // snapshot the resolved shape + field slot — the hot path becomes a
            // single shape compare + indexed field access, skipping the
            // icTable hashmap probe and the FieldInlineCache linear scan. On
            // shape miss the opcode is rewritten back and cachedFieldShape is
            // cleared; cachedFieldDeoptCount>=1 makes the demote sticky.
            // Kept parallel to cachedMethod* (rather than shared) per ticket.
            mutable const runtimeTypes::klass::ClassDefinition* cachedFieldShape = nullptr;
            mutable size_t cachedFieldIndex = static_cast<size_t>(-1);
            mutable uint8_t cachedFieldDeoptCount = 0;

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
