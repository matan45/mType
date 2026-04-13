#include "ScriptInterpreter.hpp"
#include "../errors/ParseException.hpp"
#include <iostream>
#include <filesystem>
#include <memory>
#include <algorithm>

#include "ImportManager.hpp"
#include "OptimizationService.hpp"
#include "NativeFunctionRegistry.hpp"
#include "ImportResolver.hpp"
#include "BytecodeService.hpp"
#include "BytecodeExecutor.hpp"
#include "ScriptAPI.hpp"
#include "ExecutionStrategy.hpp"
#include "BytecodeExecutionStrategy.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../errors/ReturnException.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../environment/registry/FunctionRegistry.hpp"
#include "../environment/manager/VariableManager.hpp"
#include "../environment/manager/ScopeManager.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/ArrayOperationsNative.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../analysis/UnusedVariableAnalyzer.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtime/EventLoop.hpp"
#include <fstream>

namespace services
{
    ScriptInterpreter::ScriptInterpreter()
        : executionMode(constants::ExecutionMode::BYTECODE_VM)
    {
        initializeServices();
    }

    ScriptInterpreter::ScriptInterpreter(constants::ExecutionMode mode)
        : executionMode(mode)
    {
        initializeServices();
    }

    void ScriptInterpreter::initializeServices()
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        optimizationService = std::make_unique<OptimizationService>();
        nativeRegistry = std::make_unique<NativeFunctionRegistry>(environment);
        importResolver = std::make_unique<ImportResolver>(environment);
        // Always skip strict validation since optimizations may have removed unused methods
        compiler = std::make_unique<vm::compiler::BytecodeCompiler>(environment, true);
        vm = std::make_shared<vm::runtime::VirtualMachine>(environment);

        // ScriptAPI initialized with VM support (program will be set when bytecode is loaded)
        scriptAPI = std::make_unique<ScriptAPI>(environment, vm.get(), nullptr);

        // BytecodeService needs ScriptAPI reference to update program during execution
        bytecodeService = std::make_unique<BytecodeService>(environment, optimizationService.get(), vm, scriptAPI.get());

        // Always create bytecode execution strategy
        executionStrategy = std::make_unique<BytecodeExecutionStrategy>(compiler.get(), vm, importResolver.get(), scriptAPI.get());

        // Set VM reference for reflection method/constructor invocation
        reflection::ReflectionNatives::setVM(vm);
    }

    ScriptInterpreter::~ScriptInterpreter()
    {
        // Clean up registries to prevent memory leaks in long-running programs
        cleanupRegistries();
    }

    void ScriptInterpreter::runScript(const std::string& filename)
    {
        try
        {
            auto [ast, importManager] = parseScriptFile(filename);

            // Set ImportManager on environment for clean architecture
            environment->setImportManager(importManager.get());

            // Execute the script
            value::Value result = executeScriptAST(std::move(ast));

            // Automatic cleanup after script execution to prevent memory growth
            // Only clean up interfaces that are no longer referenced
            size_t cleanedUp = cleanupUnusedInterfaces();
            if (cleanedUp > 0)
            {
                // Optional: Log cleanup activity for debugging
                // std::cout << "Cleaned up " << cleanedUp << " unused interfaces\n";
            }

            // ImportManager will be destroyed here when it goes out of scope
        }
        catch (const ParseException&)
        {
            // Re-throw to be caught by main() for centralized error handling
            // Don't print here to avoid duplicate error messages
            throw;
        }
        catch (const std::exception&)
        {
            // Re-throw to be caught by main() for centralized error handling
            // Don't print here to avoid duplicate error messages
            throw;
        }
    }

    std::pair<std::unique_ptr<ast::ASTNode>, std::unique_ptr<ImportManager>>
    ScriptInterpreter::parseScriptFile(const std::string& filename)
    {
        // Always parse and execute .mt files directly (no auto-caching)
        lexer::Lexer lexer(filename);

        // Create and configure ImportManager
        auto importManager = std::make_unique<ImportManager>();

        // Set base directory to the directory of the script file
        std::filesystem::path scriptPath(filename);
        importManager->setBaseDirectory(scriptPath.parent_path().string());

        // Set current file path to the main script file
        // This ensures imports in the main file resolve correctly
        std::string canonicalPath = std::filesystem::canonical(filename).string();
        importManager->setCurrentFilePath(canonicalPath);

        parser::Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Get the ImportManager back from Parser before it's destroyed
        auto retrievedImportManager = parser.getImportManager();

        return {std::move(ast), std::move(retrievedImportManager)};
    }

    value::Value ScriptInterpreter::executeScriptAST(std::unique_ptr<ast::ASTNode> ast)
    {
        // IMPORTANT: Resolve all imports BEFORE optimization
        // This ensures the optimizer can see and analyze imported code
        importResolver->resolveImports(ast.get());

        // MYT-49 — run the unused-variable analyzer against the resolved
        // AST BEFORE optimization (so the analyzer sees the user's
        // original code shape, not the optimized form). Push results
        // into the BytecodeCompiler's warning sink so Main.cpp's
        // post-runScript loop renders them.
        if (compiler)
        {
            auto unusedWarnings = analysis::UnusedVariableAnalyzer::analyze(ast.get());
            for (auto& w : unusedWarnings)
            {
                compiler->addWarning(std::move(w));
            }
        }

        // Apply AST optimizations
        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        // Execute using the strategy pattern
        auto result = executionStrategy->execute(ast.get());
        return result;
    }

    void ScriptInterpreter::resetForRebuild()
    {
        if (environment)
        {
            environment->resetForRebuild();
        }

        cachedBytecodeProgram.reset();

        if (vm)
        {
            vm->reset();
        }

        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(nullptr);
        }
    }

    void ScriptInterpreter::cleanupRegistries()
    {
        if (environment)
        {
            // Clean up unused interfaces to prevent memory growth
            environment->cleanupUnusedInterfaces();
            // Clear interface validation cache to free memory
            auto interfaceRegistry = environment->getInterfaceRegistry();
            if (interfaceRegistry)
            {
                interfaceRegistry->clearValidationCache();
            }
        }
    }

    size_t ScriptInterpreter::cleanupUnusedInterfaces()
    {
        if (!environment) return 0;
        return environment->cleanupUnusedInterfaces();
    }

    std::vector<std::string> ScriptInterpreter::findUnusedInterfaces() const
    {
        if (!environment) return {};
        return environment->findUnusedInterfaces();
    }

    void ScriptInterpreter::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        nativeRegistry->registerNativeFunction(name, function);
    }

    void ScriptInterpreter::registerNativeVariable(const std::string& name, const Value& value)
    {
        nativeRegistry->registerNativeVariable(name, value);
    }

    void ScriptInterpreter::registerNativeClass(const std::string& name)
    {
        nativeRegistry->registerNativeClass(name);
    }

    void ScriptInterpreter::registerNativeMethod(const std::string& className,
                                                 const std::string& methodName,
                                                 NativeFunction function,
                                                 bool isStatic)
    {
        nativeRegistry->registerNativeMethod(className, methodName, function, isStatic);
    }

    void ScriptInterpreter::registerNativeField(const std::string& className,
                                                const std::string& fieldName,
                                                const value::Value& value,
                                                bool isStatic)
    {
        nativeRegistry->registerNativeField(className, fieldName, value, isStatic);
    }

    // C++ to mType calling interface - delegated to ScriptAPI
    Value ScriptInterpreter::callFunction(const std::string& functionName, const std::vector<Value>& args)
    {
        return scriptAPI->callFunction(functionName, args);
    }

    Value ScriptInterpreter::callMethod(const Value& object, const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        return scriptAPI->callMethod(object, methodName, args);
    }

    Value ScriptInterpreter::callStaticMethod(const std::string& className, const std::string& methodName,
                                              const std::vector<Value>& args)
    {
        return scriptAPI->callStaticMethod(className, methodName, args);
    }

    Value ScriptInterpreter::callLambda(const Value& lambda, const std::vector<Value>& args)
    {
        return scriptAPI->callLambda(lambda, args);
    }

    Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
    {
        return scriptAPI->getStaticField(className, fieldName);
    }

    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName,
                                           const Value& value)
    {
        scriptAPI->setStaticField(className, fieldName, value);
    }

    Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        return scriptAPI->getVariable(variableName);
    }

    void ScriptInterpreter::setVariable(const std::string& variableName, const Value& value)
    {
        scriptAPI->setVariable(variableName, value);
    }

    Value ScriptInterpreter::createObject(const std::string& className, const std::vector<Value>& constructorArgs)
    {
        return scriptAPI->createObject(className, constructorArgs);
    }

    bool ScriptInterpreter::isObjectOfClass(const Value& object, const std::string& className)
    {
        return scriptAPI->isObjectOfClass(object, className);
    }

    std::string ScriptInterpreter::getObjectClassName(const Value& object)
    {
        return scriptAPI->getObjectClassName(object);
    }

    bool ScriptInterpreter::classImplementsInterface(const std::string& className, const std::string& interfaceName)
    {
        if (!environment)
        {
            return false;
        }

        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            return false;
        }

        // Use the full interface checking with transitive support
        auto interfaceRegistry = environment->getInterfaceRegistry();
        return classDef->implementsInterface(interfaceName, interfaceRegistry);
    }

    // Execution mode control
    void ScriptInterpreter::setExecutionMode(constants::ExecutionMode mode)
    {
        executionMode = mode;
    }

    constants::ExecutionMode ScriptInterpreter::getExecutionMode() const
    {
        return executionMode;
    }

    std::shared_ptr<environment::Environment> ScriptInterpreter::getEnvironment() const
    {
        return environment;
    }

    const std::vector<diagnostics::Diagnostic>& ScriptInterpreter::getCompilerWarnings() const
    {
        // The compiler is built lazily by initializeServices(); if a caller
        // asks for warnings before any compile has happened, fall back to a
        // shared empty vector so the reference stays valid.
        static const std::vector<diagnostics::Diagnostic> kEmpty;
        return compiler ? compiler->warnings() : kEmpty;
    }

    void ScriptInterpreter::enableDebugging()
    {
        // Enable debugging for VM
        if (vm)
        {
            vm->setDebuggingEnabled(true);
        }
    }

    void ScriptInterpreter::disableDebugging()
    {
        // Disable debugging for VM
        if (vm)
        {
            vm->setDebuggingEnabled(false);
        }
    }

    void ScriptInterpreter::setCurrentBytecodeProgram(const vm::bytecode::BytecodeProgram* program)
    {
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(program);
        }
    }

    // Parse and register classes without executing
    void ScriptInterpreter::parseAndRegisterClasses(const std::string& filename)
    {
        try
        {
            // Check if this is a compiled bytecode file
            if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".mtc")
            {
                // Load pre-compiled bytecode and register classes
                loadCompiledBytecode(filename);
                return;
            }

            // Parse the script file
            auto [ast, importManager] = parseScriptFile(filename);

            // Set ImportManager on environment
            environment->setImportManager(importManager.get());

            // IMPORTANT: Resolve all imports BEFORE compilation
            // This ensures imported classes are included in the bytecode
            importResolver->resolveImports(ast.get());

            // Compile the AST to bytecode (this registers all classes)
            // Classes are registered during compilation by ClassRegistrar
            // We don't need to execute the bytecode - just compile and cache it
            if (compiler)
            {
                cachedBytecodeProgram = std::make_unique<vm::bytecode::BytecodeProgram>(compiler->compile(ast.get()));

                // Set program reference on VM for C++ API methods (createObject, invokeMethod, etc.)
                // We use setProgram() instead of execute() to avoid running the script
                if (vm)
                {
                    vm->setProgram(cachedBytecodeProgram.get());
                }

                // Set program reference on ScriptAPI for C++ interop
                if (scriptAPI)
                {
                    scriptAPI->setBytecodeProgram(cachedBytecodeProgram.get());
                }

                // Note: Classes are already registered during compilation by ClassRegistrar
                // We do NOT execute the bytecode, so any top-level code in the script won't run
            }

            // Note: Classes are now registered in the environment's class registry
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    // Execution mode helpers
    void ScriptInterpreter::compileToFile(const std::string& sourceFile, const std::string& outputFile)
    {
        bytecodeService->compileToFile(sourceFile, outputFile);
    }

    void ScriptInterpreter::compileToFile(const std::string& sourceFile, const std::string& outputFile,
                                          const std::vector<std::string>& searchPaths,
                                          const std::unordered_map<std::string, std::string>& aliases)
    {
        ImportConfig config;
        config.searchPaths = searchPaths;
        config.aliases = aliases;
        bytecodeService->compileToFile(sourceFile, outputFile, config);
    }

    void ScriptInterpreter::runCompiledBytecode(const std::string& bytecodeFile)
    {
        bytecodeService->runCompiledBytecode(bytecodeFile);
    }

    void ScriptInterpreter::loadCompiledBytecode(const std::string& bytecodeFile)
    {
        // Load bytecode and register classes without executing
        cachedBytecodeProgram = bytecodeService->loadCompiledBytecodeWithoutExecuting(bytecodeFile);
    }

    void ScriptInterpreter::loadFromProgram(vm::bytecode::BytecodeProgram program)
    {
        // Load an already-deserialized bytecode program
        cachedBytecodeProgram = bytecodeService->loadFromProgram(std::move(program));
    }

    void ScriptInterpreter::runFromProgram(vm::bytecode::BytecodeProgram program)
    {
        // Load and execute an already-deserialized bytecode program, keep it alive
        cachedBytecodeProgram = bytecodeService->runFromProgram(std::move(program));
    }

    void ScriptInterpreter::loadLibrary(const std::string& mtcLibPath)
    {
        if (!libraryLoader) {
            libraryLoader = std::make_unique<vm::runtime::LibraryLoader>();
        }
        libraryLoader->loadLibrary(mtcLibPath, *vm, environment);
    }
}
