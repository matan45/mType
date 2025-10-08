#include "ScriptInterpreter.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/ParameterMismatchException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/TypeConversionException.hpp"
#include <iostream>
#include <filesystem>
#include <memory>
#include <algorithm>

#include "ImportManager.hpp"
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
        : executionMode(constants::ExecutionMode::AST_INTERPRETER),
          optimizationLevel(constants::OptimizationLevel::O0)
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        evaluator = std::make_unique<evaluator::Evaluator>(environment);
        compiler = std::make_unique<vm::compiler::BytecodeCompiler>(environment);
        vm = std::make_unique<vm::runtime::VirtualMachine>(environment);

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

    ScriptInterpreter::ScriptInterpreter(constants::ExecutionMode mode, constants::OptimizationLevel optLevel)
        : executionMode(mode), optimizationLevel(optLevel)
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        evaluator = std::make_unique<evaluator::Evaluator>(environment);
        compiler = std::make_unique<vm::compiler::BytecodeCompiler>(environment);
        vm = std::make_unique<vm::runtime::VirtualMachine>(environment);

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
        try
        {
            // Always parse and execute .mt files directly (no auto-caching)
            lexer::Lexer lexer(filename);

            // Create and configure ImportManager
            auto importManager = std::make_unique<ImportManager>();

            // Set base directory to the directory of the script file
            std::filesystem::path scriptPath(filename);
            importManager->setBaseDirectory(scriptPath.parent_path().string());

            // Keep a raw pointer for later use before moving to Parser
            ImportManager* importManagerPtr = importManager.get();

            parser::Parser parser(lexer, std::move(importManager));

            std::unique_ptr<ASTNode> ast;
            try
            {
                ast = parser.parseProgram();
            }
            catch (const errors::ParseException&)
            {
                throw;
            }
            catch (const std::exception&)
            {
                throw;
            }
            catch (...)
            {
                throw;
            }

            // Set ImportManager on environment for clean architecture
            environment->setImportManager(importManagerPtr);

            // Execute based on execution mode
            value::Value result;
            switch (executionMode)
            {
            case constants::ExecutionMode::AST_INTERPRETER:
                result = executeAST(ast.get());
                break;
            case constants::ExecutionMode::BYTECODE_VM:
                result = executeBytecode(ast.get());
                break;
            case constants::ExecutionMode::DUAL_VALIDATION:
                result = executeDualValidation(ast.get());
                break;
            }

            // Automatic cleanup after script execution to prevent memory growth
            // Only clean up interfaces that are no longer referenced
            size_t cleanedUp = cleanupUnusedInterfaces();
            if (cleanedUp > 0)
            {
                // Optional: Log cleanup activity for debugging
                // std::cout << "Cleaned up " << cleanedUp << " unused interfaces\n";
            }
        }
        catch (const ParseException&)
        {
            std::cerr << "Error" << std::endl;
            throw; // Re-throw to be caught by main()
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            throw; // Re-throw to be caught by main()
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
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(name, function);
    }

    void ScriptInterpreter::registerNativeVariable(const std::string& name, const Value& value)
    {
        // Create a variable definition and register it
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            name, getValueType(value), false, false);
        varDef->setValue(value);

        environment->declareVariable(name, varDef);
    }

    void ScriptInterpreter::registerNativeClass(const std::string& name)
    {
        // Create a basic class definition for native classes
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(name);

        environment->registerClass(name, classDef);
    }

    void ScriptInterpreter::registerNativeMethod(const std::string& className,
                                                 const std::string& methodName,
                                                 NativeFunction function,
                                                 bool isStatic)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(methodName, function);
    }

    void ScriptInterpreter::registerNativeField(const std::string& className,
                                                const std::string& fieldName,
                                                const value::Value& value,
                                                bool isStatic)
    {
        registerNativeVariable(fieldName, value);
    }

    // C++ to mType calling interface implementations

    // Function calling
    Value ScriptInterpreter::callFunction(const std::string& functionName, const std::vector<Value>& args)
    {
        // First try to find a regular mType function
        auto funcDef = environment->findFunction(functionName);
        if (funcDef)
        {
            return invokeFunction(funcDef, args);
        }

        // If not found, try to find a native function
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
        {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc)
            {
                return nativeFunc(args);
            }
        }

        throw errors::MethodNotFoundException(functionName, "", __FUNCTION__);
    }

    // Method calling on objects
    Value ScriptInterpreter::callMethod(const Value& object, const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot call method on non-object value", "", __FUNCTION__);
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        return evaluator->callMethodOnInstance(instance, methodName, args);
    }

    // Static method calling
    Value ScriptInterpreter::callStaticMethod(const std::string& className, const std::string& methodName,
                                              const std::vector<Value>& args)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        return invokeStaticMethod(classDef, methodName, args);
    }


    // Static field access
    Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        return field->getValue();
    }

    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName,
                                           const Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (field->isFinal())
        {
            throw errors::FinalModificationException(fieldName, className);
        }

        field->setValue(value);
    }

    // Variable access
    Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        return varDef->getValue();
    }

    void ScriptInterpreter::setVariable(const std::string& variableName, const Value& value)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        if (varDef->isFinal())
        {
            throw errors::FinalModificationException(variableName);
        }

        varDef->setValue(value);
    }

    // Object creation
    Value ScriptInterpreter::createObject(const std::string& className, const std::vector<Value>& constructorArgs)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        // Create the object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        // Find and call appropriate constructor
        auto constructor = classDef->findConstructor(constructorArgs.size());
        if (constructor)
        {
            // Set up parameters in a new scope
            environment->enterScope("constructor");

            auto params = constructor->getParameters();
            if (params.size() != constructorArgs.size())
            {
                environment->exitScope();
                throw errors::ParameterMismatchException("constructor", static_cast<int>(params.size()),
                                                         static_cast<int>(constructorArgs.size()));
            }

            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(constructorArgs[i]);
                environment->declareVariable(params[i].first, paramVar);
            }

            // Execute super initializer first (if present)
            if (constructor->hasSuperInitializer())
            {
                auto superInit = constructor->getSuperInitializer();
                if (superInit)
                {
                    // Note: ScriptInterpreter uses old evaluator without context flags
                    // This will need updating when ScriptInterpreter is refactored
                    evaluator->evaluate(superInit);
                }
            }

            // Execute constructor body if it exists
            if (constructor->getBody())
            {
                evaluator->evaluate(constructor->getBody());
            }

            environment->exitScope();
        }

        return std::static_pointer_cast<runtimeTypes::klass::ObjectInstance>(instance);
    }

    Value ScriptInterpreter::createObjectForReturn(const std::string& className,
                                                   const std::vector<Value>& constructorArgs)
    {
        // Use the existing createObject method - it already handles object creation properly
        return createObject(className, constructorArgs);
    }

    // Helper method implementations
    Value ScriptInterpreter::invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef,
                                            const std::vector<Value>& args)
    {
        // Set up function scope
        environment->enterScope(funcDef->getName(), environment::ScopeType::FUNCTION);

        // Bind parameters
        auto params = funcDef->getParameters();
        if (params.size() != args.size())
        {
            environment->exitScope();
            throw errors::ParameterMismatchException(funcDef->getName(), static_cast<int>(params.size()),
                                                     static_cast<int>(args.size()));
        }

        for (size_t i = 0; i < params.size(); ++i)
        {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }

        // Execute function body
        Value result = std::monostate{}; // void
        if (funcDef->getBody())
        {
            result = evaluator->evaluate(funcDef->getBody().get());

            // Check if there was a return value
            if (evaluator->shouldReturn())
            {
                result = evaluator->getReturnValue();
                evaluator->setReturned(false); // Reset return state
            }
        }

        environment->exitScope();
        return result;
    }

    Value ScriptInterpreter::invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                                const std::string& methodName,
                                                const std::vector<Value>& args)
    {
        auto method = classDef->getMethod(methodName);
        if (!method || !method->isStatic())
        {
            throw errors::MethodNotFoundException(methodName, classDef->getName());
        }

        // Set up method scope
        environment->enterScope(classDef->getName() + "::" + methodName, environment::ScopeType::FUNCTION);

        try
        {
            // Bind parameters
            auto params = method->getParameters();
            if (params.size() != args.size())
            {
                throw errors::ParameterMismatchException(
                    classDef->getName() + "::" + methodName, static_cast<int>(params.size()),
                    static_cast<int>(args.size()));
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(args[i]);
                environment->declareVariable(params[i].first, paramVar);
            }

            // Store current class name for static field access (same as ObjectEvaluator does)
            auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                "__current_class_name__", ValueType::STRING, classDef->getName(), false);
            environment->declareVariable("__current_class_name__", classNameVar);

            // Execute method body
            Value result = std::monostate{}; // void
            if (method->getBody())
            {
                auto apiEvaluator = std::make_unique<evaluator::Evaluator>(environment);

                // Set up method call handler for this evaluator too
                auto nativeRegistry = environment->getNativeRegistry();
                if (nativeRegistry)
                {
                    nativeRegistry->setMethodCallHandler(
                        [&apiEvaluator](std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                        const std::string& methodName,
                                        const std::vector<value::Value>& args) -> value::Value
                        {
                            return apiEvaluator->callMethodOnInstance(instance, methodName, args);
                        }
                    );
                }

                try
                {
                    result = apiEvaluator->evaluate(method->getBody());

                    // Check if there was a return value
                    if (apiEvaluator->shouldReturn())
                    {
                        result = apiEvaluator->getReturnValue();
                    }
                }
                catch (const ReturnException& returnEx)
                {
                    // Handle explicit return statements - this is the normal case for static methods with return values
                    result = returnEx.returnValue;
                }
                catch (const std::exception& evalException)
                {
                    std::cerr << "Warning: Error compiling dependency : " << evalException.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    throw;
                }
            }
            // Clean up and return
            environment->exitScope();

            return result;
        }
        catch (...)
        {
            // Clean up on exception
            environment->exitScope();
            throw;
        }
    }


    // Utility methods for native functions working with objects
    bool ScriptInterpreter::isObjectOfClass(const Value& object, const std::string& className)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            return false;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName() == className;
    }

    std::string ScriptInterpreter::getObjectClassName(const Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::TypeConversionException("Value is not an object", "unknown", "object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }


    void ScriptInterpreter::preRegisterClassDefinitions(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            std::string className = classNode->getClassName();

            // Pre-register class in environment using evaluator (for AST mode)
            evaluator->evaluate(classNode);

            // For bytecode mode: Also register static methods as global functions
            // This allows them to be called via CALL opcode with qualified names
            for (const auto& method : classNode->getMethods())
            {
                if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
                {
                    if (methodNode->getIsStatic())
                    {
                        // Static methods should already be registered by evaluating the class
                        // The evaluator registers them in the class definition
                        // We just need to ensure they're accessible for bytecode execution

                        // Get the class definition from environment
                        auto classDef = environment->findClass(className);
                        if (classDef)
                        {
                            // Check if this static method exists in the class
                            const auto& staticMethods = classDef->getStaticMethods();
                            auto it = staticMethods.find(methodNode->getName());
                            if (it != staticMethods.end())
                            {
                                // Register as a global function with qualified name
                                std::string qualifiedName = className + "::" + methodNode->getName();

                                // Use the existing static method definition from the class
                                // instead of creating a new one
                                auto funcRegistry = environment->getFunctionRegistry();
                                if (funcRegistry)
                                {
                                    // Register the method definition directly as a callable function
                                    // We'll use the method definition's body which is already managed
                                    auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                                        qualifiedName,
                                        methodNode->getReturnType(),
                                        methodNode->getParameters()
                                    );

                                    // Copy the body from the static method definition
                                    funcDef->setBody(it->second->getBodyPtr());

                                    funcRegistry->registerFunction(qualifiedName, funcDef);
                                }
                            }
                        }
                    }
                }
            }

            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        // Add other node types that may contain ClassNodes as needed
    }

    // Execution mode control
    void ScriptInterpreter::setExecutionMode(constants::ExecutionMode mode)
    {
        executionMode = mode;
    }

    void ScriptInterpreter::setOptimizationLevel(constants::OptimizationLevel level)
    {
        optimizationLevel = level;
    }

    // Execution mode helpers
    value::Value ScriptInterpreter::executeAST(ast::ASTNode* ast)
    {
        return evaluator->evaluate(ast);
    }

    value::Value ScriptInterpreter::executeBytecode(ast::ASTNode* ast)
    {
        try
        {
            // IMPORTANT: Resolve all imports before bytecode compilation
            // The bytecode compiler expects imports to be already resolved
            resolveImports(ast);

            // Compile AST to bytecode
            // The BytecodeCompiler will register all classes during compilation
            auto program = compiler->compile(ast);

            // Get the VM's event loop for async/await support
            auto* eventLoop = vm->getEventLoop();

            if (eventLoop)
            {
                // Schedule the main program as a task in the event loop
                size_t mainTaskId = eventLoop->scheduleTask(
                    [this, program]() -> value::Value {
                        return vm->execute(program);
                    }
                );

                // Set the VM reference for this task so it can set task ID before execution
                eventLoop->setTaskVM(mainTaskId, vm.get());

                // Run the event loop until all tasks complete
                eventLoop->run();

                // The task has completed - return the result
                // For now, we return void since the event loop doesn't track results
                return std::monostate{};
            }
            else
            {
                // No event loop - execute directly (synchronous mode)
                return vm->execute(program);
            }
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    value::Value ScriptInterpreter::executeDualValidation(ast::ASTNode* ast)
    {
        // Execute both and compare results
        value::Value astResult;
        value::Value vmResult;
        bool astSuccess = false;
        bool vmSuccess = false;

        std::cout << "=== DUAL VALIDATION MODE ===" << std::endl;

        // IMPORTANT: Resolve imports BEFORE any execution
        // This ensures ImportNodes have their importedAST set for both modes
        resolveImports(ast);

        // Try AST execution with original environment
        try
        {
            std::cout << "[AST] Executing..." << std::endl;
            astResult = evaluator->evaluate(ast);
            astSuccess = true;
            std::cout << "[AST] Success" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AST] Error: " << e.what() << std::endl;
        }

        // Try bytecode execution with fresh environment
        // This ensures variables from AST execution don't leak into VM execution
        try
        {
            std::cout << "[VM] Compiling and executing..." << std::endl;

            // Create fresh environment for VM
            environment::EnvironmentBuilder vmEnvBuilder;
            auto vmEnvironment = vmEnvBuilder.build();

            // Set ImportManager on the VM environment (imports are already resolved in the AST)
            vmEnvironment->setImportManager(environment->getImportManager());

            // Create fresh VM and compiler with new environment
            auto vmCompiler = std::make_unique<vm::compiler::BytecodeCompiler>(vmEnvironment);
            auto vmMachine = std::make_unique<vm::runtime::VirtualMachine>(vmEnvironment);

            // The imports are already resolved in the AST (from AST execution)
            // But we need to ensure the BytecodeCompiler can access them
            // The compiler will call registerClassesForBytecode which processes ImportNodes
            auto program = vmCompiler->compile(ast);
            vmResult = vmMachine->execute(program);
            vmSuccess = true;
            std::cout << "[VM] Success" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[VM] Error: " << e.what() << std::endl;
        }

        // Compare results
        if (astSuccess && vmSuccess)
        {
            std::cout << "[VALIDATION] Both executions succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return astResult; // Prefer AST result as ground truth
        }
        else if (astSuccess)
        {
            std::cout << "[VALIDATION] Only AST succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return astResult;
        }
        else if (vmSuccess)
        {
            std::cout << "[VALIDATION] Only VM succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return vmResult;
        }
        else
        {
            std::cout << "[VALIDATION] Both executions failed" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            throw std::runtime_error("Both execution modes failed");
        }
    }

    void ScriptInterpreter::resolveImports(ast::ASTNode* ast)
    {
        // Recursively resolve all imports in the AST
        // This ensures that all ImportNode objects have their importedAST set
        // before bytecode compilation

        if (!ast) return;

        auto importManager = environment->getImportManager();
        if (!importManager)
        {
            throw std::runtime_error("ImportManager not available for import resolution");
        }

        // Check if this is an ImportNode
        if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(ast))
        {
            std::string filePath = importNode->getFilePath();

            // Skip if already resolved
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                // Recursively resolve imports in the imported AST
                resolveImports(importNode->getImportedAST());
                return;
            }

            std::string resolvedPath = importManager->resolvePath(filePath);

            // Check for circular imports
            if (importManager->isBeingEvaluated(resolvedPath))
            {
                throw std::runtime_error("Circular import detected: " + filePath);
            }

            // Mark as being evaluated
            importManager->markAsBeingEvaluated(resolvedPath);

            try
            {
                // Parse and cache the imported AST
                ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

                if (!importedAST)
                {
                    throw std::runtime_error("Failed to parse import: " + filePath);
                }

                // Set the imported AST on the ImportNode
                importNode->setImportedAST(importedAST);

                // Recursively resolve imports in the imported file
                resolveImports(importedAST);

                // Pre-register class definitions from the imported AST
                // This is needed for bytecode compilation to find classes
                preRegisterClassDefinitions(importedAST);

                // Mark as evaluated
                importManager->markAsEvaluated(resolvedPath);
            }
            catch (...)
            {
                // Unmark as being evaluated on error
                importManager->unmarkAsBeingEvaluated(resolvedPath);
                throw;
            }

            // Unmark as being evaluated (successful completion)
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            return;
        }

        // Recursively process child nodes
        // Check for ProgramNode
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(ast))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                resolveImports(stmt.get());
            }
            return;
        }

        // Check for BlockNode
        if (auto blockNode = dynamic_cast<ast::nodes::statements::BlockNode*>(ast))
        {
            const auto& statements = blockNode->getStatements();
            for (const auto& stmt : statements)
            {
                resolveImports(stmt.get());
            }
            return;
        }

        // For other node types, we don't need to traverse further
        // Imports are typically at the top level
    }

    void ScriptInterpreter::compileToFile(const std::string& sourceFile, const std::string& outputFile)
    {
        using namespace lexer;
        using namespace parser;
        using namespace vm::compiler;

        // Parse the source file
        Lexer lexer(sourceFile);
        auto importManager = std::make_unique<ImportManager>();
        std::filesystem::path scriptPath(sourceFile);
        importManager->setBaseDirectory(scriptPath.parent_path().string());

        // Keep a raw pointer before moving
        ImportManager* importMgrPtr = importManager.get();

        Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Resolve all imports using ImportManager
        importMgrPtr->resolveAllImports(ast.get());

        // Compile to bytecode
        BytecodeCompiler bytecodeCompiler(environment);
        auto program = bytecodeCompiler.compile(ast.get());

        // Store source file path for class registration when loading
        program.setSourceFilePath(sourceFile);

        // Serialize to file
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("Could not open output file: " + outputFile);
        }
        program.serialize(outFile);
        outFile.close();

        std::cout << "Successfully compiled to " << outputFile << "\n";
        std::cout << "  Instructions: " << program.getInstructionCount() << "\n";
        std::cout << "  Classes: " << program.getClasses().size() << "\n";
    }

    namespace {
        // Helper function to convert type names to ValueType
        value::ValueType stringToValueType(const std::string& typeName) {
            if (typeName == "int") return value::ValueType::INT;
            if (typeName == "float") return value::ValueType::FLOAT;
            if (typeName == "bool") return value::ValueType::BOOL;
            if (typeName == "string") return value::ValueType::STRING;
            if (typeName == "void") return value::ValueType::VOID;
            if (typeName == "null") return value::ValueType::NULL_TYPE;
            // Default to OBJECT for class types
            return value::ValueType::OBJECT;
        }
    }

    void ScriptInterpreter::registerClassesFromMetadata(const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes)
    {
        using namespace runtimeTypes::klass;

        auto classRegistry = environment->getClassRegistry();

        // First pass: Create all ClassDefinitions
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classMap;
        for (const auto& classMeta : classes) {
            // Create generic parameters
            std::vector<ast::GenericTypeParameter> genericParams;
            for (const auto& paramName : classMeta.genericParameters) {
                genericParams.push_back(ast::GenericTypeParameter(paramName));
            }

            // Create ClassDefinition
            auto classDef = std::make_shared<ClassDefinition>(classMeta.name, genericParams);

            // Set parent class name and interfaces
            if (!classMeta.parentClassName.empty()) {
                classDef->setParentClassName(classMeta.parentClassName);
            }
            classDef->setImplementedInterfaces(classMeta.implementedInterfaces);

            classMap[classMeta.name] = classDef;
        }

        // Second pass: Link parent classes and populate members
        for (const auto& classMeta : classes) {
            auto classDef = classMap[classMeta.name];

            // Link parent class
            if (!classMeta.parentClassName.empty() && classMap.count(classMeta.parentClassName)) {
                classDef->setParentClass(classMap[classMeta.parentClassName]);
            }

            // Add instance fields
            for (const auto& fieldMeta : classMeta.instanceFields) {
                auto fieldType = stringToValueType(fieldMeta.type);
                auto accessMod = fieldMeta.isPrivate
                                     ? ast::AccessModifier::PRIVATE
                                     : (fieldMeta.isProtected
                                            ? ast::AccessModifier::PROTECTED
                                            : ast::AccessModifier::PUBLIC);

                auto fieldDef = std::make_shared<FieldDefinition>(
                    fieldMeta.name,
                    fieldType,
                    std::monostate{}, // Empty value - bytecode will initialize
                    false, // not static
                    fieldMeta.isFinal,
                    accessMod
                );
                classDef->addInstanceField(fieldMeta.name, fieldDef);
            }

            // Add static fields
            for (const auto& fieldMeta : classMeta.staticFields) {
                auto fieldType = stringToValueType(fieldMeta.type);
                auto accessMod = fieldMeta.isPrivate
                                     ? ast::AccessModifier::PRIVATE
                                     : (fieldMeta.isProtected
                                            ? ast::AccessModifier::PROTECTED
                                            : ast::AccessModifier::PUBLIC);

                auto fieldDef = std::make_shared<FieldDefinition>(
                    fieldMeta.name,
                    fieldType,
                    std::monostate{}, // Empty value - bytecode will initialize
                    true, // static
                    fieldMeta.isFinal,
                    accessMod
                );
                classDef->addStaticField(fieldMeta.name, fieldDef);
            }

            // Add instance methods
            for (const auto& methodMeta : classMeta.instanceMethods) {
                auto returnType = stringToValueType(methodMeta.returnType);
                std::vector<std::pair<std::string, value::ParameterType>> params;

                for (size_t i = 0; i < methodMeta.parameterNames.size(); ++i) {
                    auto paramType = stringToValueType(methodMeta.parameterTypes[i]);
                    params.push_back({methodMeta.parameterNames[i], value::ParameterType(paramType)});
                }

                auto accessMod = methodMeta.isPrivate
                                     ? ast::AccessModifier::PRIVATE
                                     : (methodMeta.isProtected
                                            ? ast::AccessModifier::PROTECTED
                                            : ast::AccessModifier::PUBLIC);

                auto methodDef = std::make_shared<MethodDefinition>(
                    methodMeta.name,
                    returnType,
                    params,
                    std::vector<std::pair<std::string, value::Value>>{}, // Empty arguments
                    nullptr, // No body for bytecode methods
                    false, // not static
                    accessMod
                );
                classDef->addInstanceMethod(methodMeta.name, methodDef);
            }

            // Add static methods
            for (const auto& methodMeta : classMeta.staticMethods) {
                auto returnType = stringToValueType(methodMeta.returnType);
                std::vector<std::pair<std::string, value::ParameterType>> params;

                for (size_t i = 0; i < methodMeta.parameterNames.size(); ++i) {
                    auto paramType = stringToValueType(methodMeta.parameterTypes[i]);
                    params.push_back({methodMeta.parameterNames[i], value::ParameterType(paramType)});
                }

                auto accessMod = methodMeta.isPrivate
                                     ? ast::AccessModifier::PRIVATE
                                     : (methodMeta.isProtected
                                            ? ast::AccessModifier::PROTECTED
                                            : ast::AccessModifier::PUBLIC);

                auto methodDef = std::make_shared<MethodDefinition>(
                    methodMeta.name,
                    returnType,
                    params,
                    std::vector<std::pair<std::string, value::Value>>{}, // Empty arguments
                    nullptr, // No body for bytecode methods
                    true, // static
                    accessMod
                );
                classDef->addStaticMethod(methodMeta.name, methodDef);
            }

            // Add constructors
            for (const auto& ctorMeta : classMeta.constructors) {
                std::vector<std::pair<std::string, value::ParameterType>> params;

                for (size_t i = 0; i < ctorMeta.parameterNames.size(); ++i) {
                    auto paramType = stringToValueType(ctorMeta.parameterTypes[i]);
                    params.push_back({ctorMeta.parameterNames[i], value::ParameterType(paramType)});
                }

                auto ctorDef = std::make_shared<ConstructorDefinition>(
                    params,
                    nullptr, // No body for bytecode constructors
                    ast::AccessModifier::PUBLIC
                );
                classDef->addConstructor(ctorDef);
            }

            // Register the class
            classRegistry->registerClass(classMeta.name, classDef);
        }
    }

    void ScriptInterpreter::runCompiledBytecode(const std::string& bytecodeFile)
    {
        using namespace vm::bytecode;
        using namespace vm::runtime;

        // Deserialize bytecode program
        std::ifstream inFile(bytecodeFile, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("Could not open bytecode file: " + bytecodeFile);
        }
        auto program = BytecodeProgram::deserialize(inFile);
        inFile.close();

        std::cout << "  Instructions: " << program.getInstructionCount() << "\n";
        std::cout << "  Classes: " << program.getClasses().size() << "\n";
        std::cout << "\nExecuting bytecode...\n\n";

        // Register classes from metadata
        registerClassesFromMetadata(program.getClasses());

        // Execute the bytecode
        if (!vm) {
            vm = std::make_unique<VirtualMachine>(environment);
        }
        vm->execute(program);
    }
}
