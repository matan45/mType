#include "BytecodeService.hpp"
#include "BytecodeExecutor.hpp"
#include "ImportManager.hpp"
#include "OptimizationService.hpp"
#include "ScriptAPI.hpp"
#include "../validation/AnnotationValidator.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../ast/GenericType.hpp"
#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace
{
    // Helper function to convert type names to ValueType
    value::ValueType stringToValueType(const std::string& typeName)
    {
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

        // Parse the source file
        Lexer lexer(sourceFile);
        auto importManager = std::make_unique<ImportManager>();
        std::filesystem::path scriptPath(sourceFile);
        std::string baseDir = scriptPath.parent_path().string();
        importManager->setBaseDirectory(baseDir);

        // Set current file path to the main script file
        // This ensures imports in the main file resolve correctly
        std::string canonicalPath = std::filesystem::canonical(sourceFile).string();
        importManager->setCurrentFilePath(canonicalPath);

        // Keep a raw pointer before moving
        ImportManager* importMgrPtr = importManager.get();

        Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Set ImportManager on environment
        environment->setImportManager(importMgrPtr);

        // Resolve all imports using ImportManager
        importMgrPtr->resolveAllImports(ast.get());

        // Apply AST optimizations
        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        // Compile to bytecode
        // Skip strict validation as AST optimizer may have removed unused methods
        BytecodeCompiler bytecodeCompiler(environment, true);
        auto program = bytecodeCompiler.compile(ast.get());

        // Store source file path for class registration when loading
        program.setSourceFilePath(sourceFile);

        // Serialize to file
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

        // Parse the source file
        Lexer lexer(sourceFile);
        auto importManager = std::make_unique<ImportManager>();
        std::filesystem::path scriptPath(sourceFile);
        std::string baseDir = scriptPath.parent_path().string();
        importManager->setBaseDirectory(baseDir);

        // Configure search paths and aliases from project config
        importManager->setSearchPaths(importConfig.searchPaths);
        importManager->setPathAliases(importConfig.aliases);

        // Set current file path to the main script file
        std::string canonicalPath = std::filesystem::canonical(sourceFile).string();
        importManager->setCurrentFilePath(canonicalPath);

        // Keep a raw pointer before moving
        ImportManager* importMgrPtr = importManager.get();

        Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Set ImportManager on environment
        environment->setImportManager(importMgrPtr);

        // Resolve all imports using ImportManager
        importMgrPtr->resolveAllImports(ast.get());

        // Apply AST optimizations
        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        // Compile to bytecode
        BytecodeCompiler bytecodeCompiler(environment, true);
        auto program = bytecodeCompiler.compile(ast.get());

        // Store source file path for class registration when loading
        program.setSourceFilePath(sourceFile);

        // Serialize to file
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

    void BytecodeService::runCompiledBytecode(const std::string& bytecodeFile)
    {
        using namespace vm::bytecode;
        using namespace vm::runtime;

        // Deserialize bytecode program
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

        // Register classes from metadata
        registerClassesFromMetadata(program.getClasses());

        // Set bytecode program on ScriptAPI for C++ interop
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(&program);
        }

        try
        {
            // Execute the bytecode using BytecodeExecutor utility
            BytecodeExecutor::executeProgram(vm, program);
        }
        catch (...)
        {
            // Clear program reference before rethrowing
            if (scriptAPI)
            {
                scriptAPI->setBytecodeProgram(nullptr);
            }
            throw;
        }

        // Clear program reference after execution
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(nullptr);
        }
    }

    std::unique_ptr<vm::bytecode::BytecodeProgram> BytecodeService::loadCompiledBytecodeWithoutExecuting(
        const std::string& bytecodeFile)
    {
        using namespace vm::bytecode;

        // Deserialize bytecode program
        std::ifstream inFile(bytecodeFile, std::ios::binary);
        if (!inFile)
        {
            throw std::runtime_error("Could not open bytecode file: " + bytecodeFile);
        }
        auto program = std::make_unique<BytecodeProgram>(BytecodeProgram::deserialize(inFile));
        inFile.close();

        // Register classes from metadata
        registerClassesFromMetadata(program->getClasses());

        // Set program on VM for C++ API methods
        if (vm)
        {
            vm->setProgram(program.get());
        }

        // Set program on ScriptAPI for C++ interop
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(program.get());
        }

        // Return the program so caller can keep it alive
        return program;
    }

    void BytecodeService::registerClassesFromMetadata(
        const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes)
    {
        using namespace runtimeTypes::klass;

        auto classRegistry = environment->getClassRegistry();

        // First pass: Create all ClassDefinitions
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classMap;
        createClassDefinitionsFirstPass(classes, classMap);

        // Second pass: Link parent classes and populate members
        for (const auto& classMeta : classes)
        {
            auto classDef = classMap[classMeta.name];
            populateClassFromMetadata(classMeta, classDef, classMap);

            // Validate annotations (e.g., @Script)
            // NOTE: Annotations are now serialized in ClassMetadata for bytecode mode
            ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);

            // Register the class
            classRegistry->registerClass(classMeta.name, classDef);
        }
    }

    void BytecodeService::createClassDefinitionsFirstPass(
        const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes,
        std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap)
    {
        using namespace runtimeTypes::klass;

        for (const auto& classMeta : classes)
        {
            // Create generic parameters
            std::vector<ast::GenericTypeParameter> genericParams;
            for (const auto& paramName : classMeta.genericParameters)
            {
                genericParams.push_back(ast::GenericTypeParameter(paramName));
            }

            // Create ClassDefinition
            auto classDef = std::make_shared<ClassDefinition>(classMeta.name, genericParams);

            // Set parent class name and interfaces
            if (!classMeta.parentClassName.empty())
            {
                classDef->setParentClassName(classMeta.parentClassName);
            }
            classDef->setImplementedInterfaces(classMeta.implementedInterfaces);

            // Restore annotations from bytecode metadata
            for (const auto& annotData : classMeta.annotations)
            {
                // Create annotation node from metadata
                // Convert argument pairs back to parameter map
                std::unordered_map<std::string, std::string> params;
                for (const auto& [key, value] : annotData.arguments)
                {
                    params[key] = value;
                }

                auto annotNode = std::make_shared<ast::nodes::annotations::AnnotationNode>(
                    annotData.name,
                    params,
                    annotData.location  // Use deserialized source location
                );
                classDef->addAnnotation(annotNode);
            }

            classMap[classMeta.name] = classDef;
        }
    }

    void BytecodeService::populateClassFromMetadata(
        const vm::bytecode::BytecodeProgram::ClassMetadata& classMeta,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap)
    {
        // Link parent class
        if (!classMeta.parentClassName.empty() && classMap.count(classMeta.parentClassName))
        {
            classDef->setParentClass(classMap.at(classMeta.parentClassName));
        }

        // Add fields, methods, and constructors
        addFieldsToClass(classMeta.instanceFields, classDef, false);
        addFieldsToClass(classMeta.staticFields, classDef, true);
        addMethodsToClass(classMeta.instanceMethods, classDef, false);
        addMethodsToClass(classMeta.staticMethods, classDef, true);
        addConstructorsToClass(classMeta.constructors, classDef);
    }

    void BytecodeService::addFieldsToClass(
        const std::vector<vm::bytecode::BytecodeProgram::FieldMetadata>& fields,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        bool isStatic)
    {
        using namespace runtimeTypes::klass;

        for (const auto& fieldMeta : fields)
        {
            auto fieldType = stringToValueType(fieldMeta.type);
            auto accessMod = fieldMeta.isPrivate
                                 ? ast::AccessModifier::PRIVATE
                                 : (fieldMeta.isProtected
                                        ? ast::AccessModifier::PROTECTED
                                        : ast::AccessModifier::PUBLIC);

            // Initialize static fields with default values based on type
            value::Value defaultValue;
            if (isStatic)
            {
                switch (fieldType)
                {
                case value::ValueType::INT:
                    defaultValue = 0;
                    break;
                case value::ValueType::FLOAT:
                    defaultValue = 0.0f;
                    break;
                case value::ValueType::STRING:
                    defaultValue = std::string("");
                    break;
                case value::ValueType::BOOL:
                    defaultValue = false;
                    break;
                default:
                    defaultValue = std::monostate{}; // null for objects
                    break;
                }
            }
            else
            {
                // Instance fields don't need default values here (initialized in constructor)
                defaultValue = std::monostate{};
            }

            auto fieldDef = std::make_shared<FieldDefinition>(
                fieldMeta.name,
                fieldType,
                defaultValue,
                isStatic,
                fieldMeta.isFinal,
                accessMod
            );

            if (isStatic)
            {
                classDef->addStaticField(fieldMeta.name, fieldDef);
            }
            else
            {
                classDef->addInstanceField(fieldMeta.name, fieldDef);
            }
        }
    }

    void BytecodeService::addMethodsToClass(
        const std::vector<vm::bytecode::BytecodeProgram::MethodMetadata>& methods,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        bool isStatic)
    {
        using namespace runtimeTypes::klass;

        for (const auto& methodMeta : methods)
        {
            auto returnType = stringToValueType(methodMeta.returnType);
            std::vector<std::pair<std::string, value::ParameterType>> params;

            for (size_t i = 0; i < methodMeta.parameterNames.size(); ++i)
            {
                auto paramType = stringToValueType(methodMeta.parameterTypes[i]);
                const std::string& typeStr = methodMeta.parameterTypes[i];

                // Preserve generic type parameters and class names for proper type signature matching
                if (paramType == value::ValueType::OBJECT) {
                    // Keep the original type string (e.g., "T", "Node<T>", "Int") as className
                    value::ParameterType paramTypeWithClass(paramType);
                    paramTypeWithClass.className = typeStr;
                    params.push_back({methodMeta.parameterNames[i], paramTypeWithClass});
                } else {
                    params.push_back({methodMeta.parameterNames[i], value::ParameterType(paramType)});
                }
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
                nullptr, // No body for bytecode methods
                isStatic,
                accessMod
            );

            // Set method modifiers from metadata
            methodDef->setAbstract(methodMeta.isAbstract);
            methodDef->setFinal(methodMeta.isFinal);

            // Track abstract methods in the class definition
            if (methodMeta.isAbstract) {
                classDef->addAbstractMethod(methodMeta.name);
            }

            if (isStatic)
            {
                classDef->addStaticMethod(methodMeta.name, methodDef);
            }
            else
            {
                classDef->addInstanceMethod(methodMeta.name, methodDef);
            }
        }
    }

    void BytecodeService::addConstructorsToClass(
        const std::vector<vm::bytecode::BytecodeProgram::ConstructorMetadata>& constructors,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        using namespace runtimeTypes::klass;

        for (const auto& ctorMeta : constructors)
        {
            std::vector<std::pair<std::string, value::ParameterType>> params;

            for (size_t i = 0; i < ctorMeta.parameterNames.size(); ++i)
            {
                auto paramType = stringToValueType(ctorMeta.parameterTypes[i]);
                const std::string& typeStr = ctorMeta.parameterTypes[i];

                // Preserve generic type parameters and class names for proper type signature matching
                if (paramType == value::ValueType::OBJECT) {
                    // Keep the original type string (e.g., "T", "Node<T>", "Int") as className
                    value::ParameterType paramTypeWithClass(paramType);
                    paramTypeWithClass.className = typeStr;
                    params.push_back({ctorMeta.parameterNames[i], paramTypeWithClass});
                } else {
                    params.push_back({ctorMeta.parameterNames[i], value::ParameterType(paramType)});
                }
            }

            auto ctorDef = std::make_shared<ConstructorDefinition>(
                params,
                nullptr, // No body for bytecode constructors
                ast::AccessModifier::PUBLIC
            );
            classDef->addConstructor(ctorDef);
        }
    }
}
