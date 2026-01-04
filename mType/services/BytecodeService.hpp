#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../environment/Environment.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    class OptimizationService;
    class ScriptAPI;

    struct ImportConfig
    {
        std::vector<std::string> searchPaths;
        std::unordered_map<std::string, std::string> aliases;
    };

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
        ScriptAPI* scriptAPI;  // For updating bytecode program reference

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
                       std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                       ScriptAPI* api = nullptr);
        ~BytecodeService();

        // Compile source file to bytecode file
        void compileToFile(const std::string& sourceFile, const std::string& outputFile);

        // Compile source file with custom import configuration
        void compileToFile(const std::string& sourceFile, const std::string& outputFile,
                          const ImportConfig& importConfig);

        // Load and execute bytecode file
        void runCompiledBytecode(const std::string& bytecodeFile);

        // Load bytecode file and register classes without executing
        std::unique_ptr<vm::bytecode::BytecodeProgram> loadCompiledBytecodeWithoutExecuting(const std::string& bytecodeFile);
    };
}
