#include "ScriptInterpreter.hpp"
#include "../errors/ParseException.hpp"
#include "../exception/SuspendException.hpp"
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
#include "ASTExecutionStrategy.hpp"
#include "BytecodeExecutionStrategy.hpp"
#include "DualValidationStrategy.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../errors/ReturnException.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../runtime/EventLoop.hpp"
#include <fstream>

namespace services
{
    ScriptInterpreter::ScriptInterpreter()
        : executionMode(constants::ExecutionMode::AST_INTERPRETER)
    {
        initializeServices(constants::OptimizationLevel::Debug);
    }

    ScriptInterpreter::ScriptInterpreter(constants::ExecutionMode mode, constants::OptimizationLevel optLevel)
        : executionMode(mode)
    {
        initializeServices(optLevel);
    }

    void ScriptInterpreter::initializeServices(constants::OptimizationLevel optLevel)
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        evaluator = std::make_unique<evaluator::Evaluator>(environment);
        optimizationService = std::make_unique<OptimizationService>(optLevel);
        nativeRegistry = std::make_unique<NativeFunctionRegistry>(environment);
        importResolver = std::make_unique<ImportResolver>(environment);
        bool skipStrictValidation = (optLevel == constants::OptimizationLevel::Release);
        compiler = std::make_unique<vm::compiler::BytecodeCompiler>(environment, skipStrictValidation, optLevel);
        vm = std::make_shared<vm::runtime::VirtualMachine>(environment);
        // ScriptAPI initialized with VM support (program will be set when bytecode is loaded)
        scriptAPI = std::make_unique<ScriptAPI>(environment, evaluator.get(), vm.get(), nullptr);
        // BytecodeService needs ScriptAPI reference to update program during execution
        bytecodeService = std::make_unique<BytecodeService>(environment, optimizationService.get(), vm, scriptAPI.get());

        // Create appropriate execution strategy based on execution mode
        switch (executionMode)
        {
        case constants::ExecutionMode::AST_INTERPRETER:
            executionStrategy = std::make_unique<ASTExecutionStrategy>(evaluator.get());
            break;
        case constants::ExecutionMode::BYTECODE_VM:
            executionStrategy = std::make_unique<BytecodeExecutionStrategy>(compiler.get(), vm, importResolver.get(), scriptAPI.get());
            break;
        case constants::ExecutionMode::DUAL_VALIDATION:
            executionStrategy = std::make_unique<DualValidationStrategy>(evaluator.get(), environment, importResolver.get(), optimizationService.get());
            break;
        default:
            throw std::runtime_error("Unknown execution mode");
        }

        // Set up method call handler for native functions
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry)
        {
            nativeRegistry->setMethodCallHandler(
                [this](std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                       const std::string& methodName,
                       const std::vector<value::Value>& args) -> value::Value
                {
                    return evaluator->callMethodOnInstance(instance, methodName, args);
                }
            );
        }
    }

    ScriptInterpreter::~ScriptInterpreter()
    {
        // Clean up registries to prevent memory leaks in long-running programs
        cleanupRegistries();
    }

    void ScriptInterpreter::runScript(const std::string& filename)
    {
        std::cerr << "[DEBUG C++] ScriptInterpreter::runScript() entered, filename=" << filename << "\n";
        try
        {
            std::cerr << "[DEBUG C++] About to call parseScriptFile()\n";
            auto [ast, importManager] = parseScriptFile(filename);
            std::cerr << "[DEBUG C++] parseScriptFile() returned successfully\n";

            // Set ImportManager on environment for clean architecture
            std::cerr << "[DEBUG C++] Setting ImportManager on environment\n";
            environment->setImportManager(importManager.get());

            // Execute the script
            std::cerr << "[DEBUG C++] About to call executeScriptAST()\n";
            value::Value result = executeScriptAST(std::move(ast));
            std::cerr << "[DEBUG C++] executeScriptAST() returned successfully\n";

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

        // Apply AST optimizations
        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        // Execute using the strategy pattern
        auto result = executionStrategy->execute(ast.get());
        return result;
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

    // Execution mode control
    void ScriptInterpreter::setExecutionMode(constants::ExecutionMode mode)
    {
        executionMode = mode;
    }

    constants::ExecutionMode ScriptInterpreter::getExecutionMode() const
    {
        return executionMode;
    }

    void ScriptInterpreter::setOptimizationLevel(constants::OptimizationLevel level)
    {
        if (optimizationService)
        {
            optimizationService->setOptimizationLevel(level);
        }
    }

    constants::OptimizationLevel ScriptInterpreter::getOptimizationLevel() const
    {
        return optimizationService ? optimizationService->getOptimizationLevel()
                                   : constants::OptimizationLevel::Debug;
    }

    std::shared_ptr<environment::Environment> ScriptInterpreter::getEnvironment() const
    {
        return environment;
    }

    evaluator::Evaluator* ScriptInterpreter::getEvaluator() const
    {
        return evaluator.get();
    }

    void ScriptInterpreter::enableDebugging()
    {
        // Enable debugging for evaluator (AST mode)
        if (evaluator)
        {
            auto context = evaluator->getContext();
            if (context)
            {
                context->setDebuggingEnabled(true);
            }
        }

        // Enable debugging for VM (bytecode mode)
        if (vm)
        {
            vm->setDebuggingEnabled(true);
        }
    }

    void ScriptInterpreter::disableDebugging()
    {
        // Disable debugging for evaluator (AST mode)
        if (evaluator)
        {
            auto context = evaluator->getContext();
            if (context)
            {
                context->setDebuggingEnabled(false);
            }
        }

        // Disable debugging for VM (bytecode mode)
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

            // For bytecode mode, compile the AST to register classes
            // Classes are registered during compilation by ClassRegistrar
            // We don't need to execute the bytecode - just compile and cache it
            if (executionMode == constants::ExecutionMode::BYTECODE_VM && compiler)
            {
                // Compile the AST to bytecode (this registers all classes)
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
            else
            {
                // For AST mode, we need to evaluate the AST to register classes
                // Class registration happens during evaluation via ClassDeclarationHandler
                if (executionStrategy)
                {
                    executionStrategy->execute(ast.get());
                }
                else if (evaluator)
                {
                    evaluator->evaluate(ast.get());
                }
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

    void ScriptInterpreter::runCompiledBytecode(const std::string& bytecodeFile)
    {
        bytecodeService->runCompiledBytecode(bytecodeFile);
    }

    void ScriptInterpreter::loadCompiledBytecode(const std::string& bytecodeFile)
    {
        // Load bytecode and register classes without executing
        cachedBytecodeProgram = bytecodeService->loadCompiledBytecodeWithoutExecuting(bytecodeFile);
    }
}
