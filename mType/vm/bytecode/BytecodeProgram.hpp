#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <iosfwd>
#include "OpCode.hpp"

namespace vm::bytecode
{
    /**
     * Represents a compiled bytecode program with constant pool and metadata
     * This is the output of the BytecodeCompiler and input to the VirtualMachine
     */
    class BytecodeProgram
    {
    public:
        /**
         * Instruction structure: opcode + operands
         */
        struct Instruction {
            OpCode opcode;
            std::vector<uint32_t> operands;

            Instruction();
            Instruction(OpCode op);
            Instruction(OpCode op, uint32_t operand1);
            Instruction(OpCode op, uint32_t operand1, uint32_t operand2);
            Instruction(OpCode op, std::vector<uint32_t> ops);
        };

        /**
         * Constant pool for storing literal values
         */
        struct ConstantPool {
            std::vector<int> integers;
            std::vector<double> floats;
            std::vector<std::string> strings;
            std::unordered_map<std::string, size_t> stringIndexMap;

            size_t addInteger(int value);
            size_t addFloat(double value);
            size_t addString(const std::string& value);
            int getInteger(size_t index) const;
            double getFloat(size_t index) const;
            const std::string& getString(size_t index) const;
        };

        /**
         * Function metadata for compiled functions
         */
        struct FunctionMetadata {
            std::string name;
            size_t startOffset;
            size_t instructionCount;
            size_t localCount;
            size_t parameterCount;
            std::vector<std::string> parameterNames;
            std::string returnType;
            bool isStatic = false;
            bool isNative = false;
        };

        /**
         * Source location mapping for debugging
         */
        struct SourceLocation {
            uint32_t line;
            uint32_t column;
            std::string filename;
        };

        /**
         * Class metadata for registration when loading cached bytecode
         */
        struct FieldMetadata {
            std::string name;
            std::string type;
            bool isStatic;
            bool isFinal;
            bool isPrivate;
            bool isProtected;
        };

        struct MethodMetadata {
            std::string name;
            std::string returnType;
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            bool isStatic;
            bool isFinal;
            bool isPrivate;
            bool isProtected;
            size_t startOffset;  // Where the method bytecode starts
        };

        struct ConstructorMetadata {
            std::vector<std::string> parameterTypes;
            std::vector<std::string> parameterNames;
            size_t startOffset;  // Where the constructor bytecode starts
        };

        struct ClassMetadata {
            std::string name;
            std::string parentClassName;
            std::vector<std::string> implementedInterfaces;
            std::vector<std::string> genericParameters;
            std::vector<FieldMetadata> instanceFields;
            std::vector<FieldMetadata> staticFields;
            std::vector<MethodMetadata> instanceMethods;
            std::vector<MethodMetadata> staticMethods;
            std::vector<ConstructorMetadata> constructors;
        };

    private:
        std::vector<Instruction> instructions;
        ConstantPool constantPool;
        std::unordered_map<std::string, FunctionMetadata> functions;
        std::unordered_map<size_t, SourceLocation> sourceLocations;
        std::vector<ClassMetadata> classes;  // Class metadata for cached bytecode
        size_t entryPoint;
        std::string sourceFilePath;  // For class registration when loading cached bytecode

    public:
        BytecodeProgram();

        // Instruction Management
        void emit(OpCode opcode);
        void emit(OpCode opcode, uint32_t operand);
        void emit(OpCode opcode, uint32_t operand1, uint32_t operand2);
        void emit(OpCode opcode, const std::vector<uint32_t>& operands);

        size_t getCurrentOffset() const;
        void patchJump(size_t instructionIndex, uint32_t targetOffset);

        const Instruction& getInstruction(size_t offset) const;
        const std::vector<Instruction>& getInstructions() const;
        size_t getInstructionCount() const;

        // Constant Pool Management
        ConstantPool& getConstantPool();
        const ConstantPool& getConstantPool() const;

        // Function Management
        void registerFunction(const std::string& name, const FunctionMetadata& metadata);
        const FunctionMetadata* getFunction(const std::string& name) const;
        const std::unordered_map<std::string, FunctionMetadata>& getFunctions() const;

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
    };
}

