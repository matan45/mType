#pragma once
#include <string>
#include <memory>
#include <vector>
#include "../environment/Environment.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    class OptimizationService;

    /**
     * Service for bytecode compilation and execution
     * Handles compiling mType source to bytecode and loading/executing bytecode files
     */
    class BytecodeService
    {
    private:
        std::shared_ptr<environment::Environment> environment;
        OptimizationService* optimizationService;
        std::shared_ptr<vm::runtime::VirtualMachine> vm;

        // Register classes from bytecode metadata into environment
        void registerClassesFromMetadata(const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes);

        // Helper functions for class registration
        void createClassDefinitionsFirstPass(
            const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes,
            std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap);

        void populateClassFromMetadata(
            const vm::bytecode::BytecodeProgram::ClassMetadata& classMeta,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            const std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap);

        void addFieldsToClass(
            const std::vector<vm::bytecode::BytecodeProgram::FieldMetadata>& fields,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            bool isStatic);

        void addMethodsToClass(
            const std::vector<vm::bytecode::BytecodeProgram::MethodMetadata>& methods,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            bool isStatic);

        void addConstructorsToClass(
            const std::vector<vm::bytecode::BytecodeProgram::ConstructorMetadata>& constructors,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);

    public:
        BytecodeService(std::shared_ptr<environment::Environment> env,
                       OptimizationService* optService,
                       std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine);
        ~BytecodeService();

        // Compile source file to bytecode file
        void compileToFile(const std::string& sourceFile, const std::string& outputFile);

        // Load and execute bytecode file
        void runCompiledBytecode(const std::string& bytecodeFile);
    };
}
