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
        };

        struct Instruction
        {
            OpCode opcode;
            std::vector<uint64_t> operands;
            uint8_t flags = 0;

            Instruction();
            Instruction(OpCode op);
            Instruction(OpCode op, uint64_t operand1);
            Instruction(OpCode op, uint64_t operand1, uint64_t operand2);
            Instruction(OpCode op, std::vector<uint64_t> ops);
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
        std::vector<GlobalVariableMetadata> globalVariables;
        ExceptionTable globalExceptionTable;
        size_t entryPoint;
        size_t topLevelLocalCount = 0;
        std::string sourceFilePath;

        mutable std::unordered_set<size_t> fusionUnsafeTargets;
        mutable bool fusionUnsafeTargetsBuilt = false;
        void buildFusionUnsafeTargets() const;
        void invalidateFusionUnsafeTargets() const { fusionUnsafeTargetsBuilt = false; fusionUnsafeTargets.clear(); }

        mutable std::unordered_map<size_t, CachedInstructionState> cachedStates;

    public:
        BytecodeProgram();

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
