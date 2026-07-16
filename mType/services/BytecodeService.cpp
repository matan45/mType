#include "BytecodeService.hpp"
#include <cstddef>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "BytecodeExecutor.hpp"
#include "BytecodeProgramBinding.hpp"
#include "ImportManager.hpp"
#include "OptimizationService.hpp"
#include "ScriptAPI.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../project/mtclib/MtcLibSerializer.hpp"

namespace services
{
    BytecodeService::BytecodeService(std::shared_ptr<environment::Environment> env,
                                     OptimizationService* optService,
                                     std::shared_ptr<vm::runtime::VirtualMachine> virtualMachine,
                                     ScriptAPI* api)
        : environment(env), optimizationService(optService), vm(virtualMachine), scriptAPI(api)
    {
    }

    BytecodeService::~BytecodeService() = default;

    void BytecodeService::compileToFile(const std::string& sourceFile, const std::string& outputFile)
    {
        using namespace lexer;
        using namespace parser;
        using namespace vm::compiler;

        Lexer lexer(sourceFile);
        auto importManager = std::make_unique<ImportManager>();
        std::filesystem::path scriptPath(sourceFile);
        std::string baseDir = scriptPath.parent_path().string();
        importManager->setBaseDirectory(baseDir);

        // Anchor imports in the main file to its own resolved location.
        std::string canonicalPath = std::filesystem::canonical(sourceFile).string();
        importManager->setCurrentFilePath(canonicalPath);

        ImportManager* importMgrPtr = importManager.get();

        Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        environment->setImportManager(importMgrPtr);
        importMgrPtr->resolveAllImports(ast.get());

        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        // Skip strict validation: the AST optimizer may have removed unused methods.
        BytecodeCompiler bytecodeCompiler(environment, true);
        auto program = bytecodeCompiler.compile(ast.get());

        program.setSourceFilePath(sourceFile);

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile)
        {
            throw std::runtime_error("Could not open output file: " + outputFile);
        }
        program.serialize(outFile);
        outFile.close();

        std::cout << "Successfully compiled to " << outputFile << "\n";
        std::cout << "  Instructions: " << program.getInstructionCount() << "\n";
        std::cout << "  Classes: " << program.getClasses().size() << "\n";
    }

    void BytecodeService::compileToFile(const std::string& sourceFile, const std::string& outputFile,
                                        const ImportConfig& importConfig)
    {
        using namespace lexer;
        using namespace parser;
        using namespace vm::compiler;

        Lexer lexer(sourceFile);
        auto importManager = std::make_unique<ImportManager>();
        std::filesystem::path scriptPath(sourceFile);
        std::string baseDir = scriptPath.parent_path().string();
        importManager->setBaseDirectory(baseDir);

        importManager->setSearchPaths(importConfig.searchPaths);
        importManager->setPathAliases(importConfig.aliases);

        std::string canonicalPath = std::filesystem::canonical(sourceFile).string();
        importManager->setCurrentFilePath(canonicalPath);

        ImportManager* importMgrPtr = importManager.get();

        Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        environment->setImportManager(importMgrPtr);
        importMgrPtr->resolveAllImports(ast.get());

        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        BytecodeCompiler bytecodeCompiler(environment, true);
        auto program = bytecodeCompiler.compile(ast.get());

        program.setSourceFilePath(sourceFile);

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile)
        {
            throw std::runtime_error("Could not open output file: " + outputFile);
        }
        program.serialize(outFile);
        outFile.close();

        std::cout << "Successfully compiled to " << outputFile << "\n";
        std::cout << "  Instructions: " << program.getInstructionCount() << "\n";
        std::cout << "  Classes: " << program.getClasses().size() << "\n";
    }

    void BytecodeService::runCompiledBytecode(
        const std::string& bytecodeFile,
        std::unique_ptr<vm::bytecode::BytecodeProgram>& owner)
    {
        using namespace vm::bytecode;
        using namespace vm::runtime;

        std::ifstream inFile(bytecodeFile, std::ios::binary);
        if (!inFile)
        {
            throw std::runtime_error("Could not open bytecode file: " + bytecodeFile);
        }
        auto program = BytecodeProgram::deserialize(inFile);
        inFile.close();

        std::cout << "  Instructions: " << program.getInstructionCount() << "\n";
        std::cout << "  Classes: " << program.getClasses().size() << "\n";
        std::cout << "\nExecuting bytecode...\n\n";

        auto replacement =
            std::make_unique<BytecodeProgram>(std::move(program));
        // Class publication can replace ClassDefinition objects referenced by
        // JIT/IC state. Invalidate that state first while the old owner lives.
        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        registerAnnotationsFromMetadata(
            replacement->getAnnotationDeclarations());
        registerClassesFromMetadata(replacement->getClasses());
        registerInterfacesFromMetadata(replacement->getInterfaces());
        BytecodeProgramBinding::replace(
            vm, scriptAPI, owner, std::move(replacement));
        BytecodeExecutor::executeProgram(vm, *owner);
    }

    void BytecodeService::loadCompiledBytecodeWithoutExecuting(
        const std::string& bytecodeFile,
        std::unique_ptr<vm::bytecode::BytecodeProgram>& owner)
    {
        using namespace vm::bytecode;

        // Deserialize raw .mtc bytecode or unwrap a .mtcLib container.
        std::ifstream inFile(bytecodeFile, std::ios::binary);
        if (!inFile)
        {
            throw std::runtime_error("Could not open bytecode file: " + bytecodeFile);
        }
        char magic[6] = {};
        inFile.read(magic, sizeof(magic));
        const bool isMtcLib = inFile.gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
                              std::memcmp(magic, "MTCLIB", sizeof(magic)) == 0;

        inFile.clear();
        inFile.seekg(0, std::ios::beg);

        auto program = isMtcLib
            ? std::make_unique<BytecodeProgram>(
                std::move(project::mtclib::MtcLibSerializer::deserialize(inFile).bytecodeProgram))
            : std::make_unique<BytecodeProgram>(BytecodeProgram::deserialize(inFile));
        inFile.close();

        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        registerAnnotationsFromMetadata(program->getAnnotationDeclarations());
        registerClassesFromMetadata(program->getClasses());
        registerInterfacesFromMetadata(program->getInterfaces());

        BytecodeProgramBinding::replace(
            vm, scriptAPI, owner, std::move(program));
    }

    void BytecodeService::runFromProgram(
        vm::bytecode::BytecodeProgram program,
        std::unique_ptr<vm::bytecode::BytecodeProgram>& owner)
    {
        auto result = std::make_unique<vm::bytecode::BytecodeProgram>(std::move(program));

        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        registerAnnotationsFromMetadata(result->getAnnotationDeclarations());
        registerClassesFromMetadata(result->getClasses());
        registerInterfacesFromMetadata(result->getInterfaces());

        BytecodeProgramBinding::replace(
            vm, scriptAPI, owner, std::move(result));

        // Execute only after the caller's durable owner and both raw bindings
        // agree, so completion, failure, or suspension cannot expose a local.
        BytecodeExecutor::executeProgram(vm, *owner);
    }

    void BytecodeService::loadFromProgram(
        vm::bytecode::BytecodeProgram program,
        std::unique_ptr<vm::bytecode::BytecodeProgram>& owner)
    {
        auto result = std::make_unique<vm::bytecode::BytecodeProgram>(std::move(program));

        BytecodeProgramBinding::clearCurrent(vm, scriptAPI);
        registerAnnotationsFromMetadata(result->getAnnotationDeclarations());
        registerClassesFromMetadata(result->getClasses());
        registerInterfacesFromMetadata(result->getInterfaces());

        BytecodeProgramBinding::replace(
            vm, scriptAPI, owner, std::move(result));
    }
}
