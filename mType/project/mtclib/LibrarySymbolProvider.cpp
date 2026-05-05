#include "LibrarySymbolProvider.hpp"
#include <cstddef>
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../runtimeTypes/klass/InterfaceRegistry.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../runtimeTypes/global/FunctionDefinition.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/registry/FunctionRegistry.hpp"
#include "../../types/UnifiedType.hpp"
#include "../../value/ParameterType.hpp"
#include "../../ast/GenericTypeParameter.hpp"

namespace project::mtclib
{
    namespace
    {
        value::ParameterType stringToParameterType(const std::string& typeName)
        {
            if (typeName == "int") return value::ParameterType(value::ValueType::INT);
            if (typeName == "float") return value::ParameterType(value::ValueType::FLOAT);
            if (typeName == "bool") return value::ParameterType(value::ValueType::BOOL);
            if (typeName == "string") return value::ParameterType(value::ValueType::STRING);
            if (typeName == "void") return value::ParameterType(value::ValueType::VOID);
            return value::ParameterType::forClass(typeName);
        }

        value::ValueType stringToValueType(const std::string& typeName)
        {
            if (typeName == "int") return value::ValueType::INT;
            if (typeName == "float") return value::ValueType::FLOAT;
            if (typeName == "bool") return value::ValueType::BOOL;
            if (typeName == "string") return value::ValueType::STRING;
            if (typeName == "void") return value::ValueType::VOID;
            return value::ValueType::OBJECT;
        }
    }

    void LibrarySymbolProvider::registerLibrarySymbols(
        const MtcLibProgram& library,
        std::shared_ptr<environment::Environment> environment)
    {
        // No filter — register everything
        registerLibrarySymbols(library, environment, {});
    }

    void LibrarySymbolProvider::registerLibrarySymbols(
        const MtcLibProgram& library,
        std::shared_ptr<environment::Environment> environment,
        const std::vector<std::string>& selectedSymbols)
    {
        registerLibrarySymbols(library, environment, selectedSymbols, {});
    }

    void LibrarySymbolProvider::registerLibrarySymbols(
        const MtcLibProgram& library,
        std::shared_ptr<environment::Environment> environment,
        const std::vector<std::string>& selectedSymbols,
        const std::unordered_map<std::string, std::string>& aliases)
    {
        const auto& program = library.bytecodeProgram;

        // Build filter set (empty = allow all)
        std::unordered_set<std::string> filter(selectedSymbols.begin(), selectedSymbols.end());

        // Register in order: interfaces first (classes may implement them)
        registerInterfaceStubs(program, environment, filter, aliases);
        registerClassStubs(program, environment, filter, aliases);
        registerFunctionStubs(program, environment, filter, aliases);
    }

    void LibrarySymbolProvider::registerClassStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const std::unordered_set<std::string>& filter,
        const std::unordered_map<std::string, std::string>& aliases)
    {
        using namespace runtimeTypes::klass;

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) return;

        // Two-pass approach: first create all stubs, then link parent classes
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classMap;

        // === Pass 1: Create all ClassDefinition stubs ===
        for (const auto& classMeta : program.getClasses()) {
            // Skip if not in filter (when filter is active)
            if (!filter.empty() && filter.find(classMeta.name) == filter.end()) continue;
            // Detect name collision with already-registered class from another library.
            // Design choice: selective imports (user explicitly named the symbol) throw on
            // conflict so the user gets a clear error. Wildcard imports silently skip
            // duplicates ("first-loaded wins") because transitive stdlib classes like
            // Object/Exception commonly appear in multiple libraries — erroring on those
            // would break every multi-library project. A future improvement could emit a
            // diagnostic warning for non-stdlib wildcard collisions.
            if (classRegistry->findClass(classMeta.name)) {
                if (!filter.empty()) {
                    throw std::runtime_error(
                        "Symbol conflict: class '" + classMeta.name +
                        "' is already defined. Use selective import to choose which library to use.");
                }
                continue;  // Wildcard import: skip silently (first-loaded wins)
            }

            // Create generic parameters
            std::vector<ast::GenericTypeParameter> genericParams;
            for (const auto& paramName : classMeta.genericParameters) {
                genericParams.push_back(ast::GenericTypeParameter(paramName));
            }

            // Create ClassDefinition stub
            auto classDef = std::make_shared<ClassDefinition>(classMeta.name, genericParams);

            if (!classMeta.parentClassName.empty()) {
                classDef->setParentClassName(classMeta.parentClassName);
            }
            classDef->setImplementedInterfaces(classMeta.implementedInterfaces);
            classDef->setAbstract(classMeta.isAbstract);
            classDef->setFinal(classMeta.isFinal);
            classDef->setValueClass(classMeta.isValueClass);

            // Add field stubs
            auto addFields = [&](const std::vector<vm::bytecode::BytecodeProgram::FieldMetadata>& fields, bool isStatic) {
                for (const auto& fieldMeta : fields) {
                    auto fieldType = ::types::UnifiedType::classType(fieldMeta.type);
                    auto fieldDef = std::make_shared<FieldDefinition>(
                        fieldMeta.name,
                        value::ValueType::OBJECT,
                        fieldType,
                        std::monostate{},
                        isStatic,
                        fieldMeta.isFinal,
                        fieldMeta.isPrivate ? ast::AccessModifier::PRIVATE
                            : (fieldMeta.isProtected ? ast::AccessModifier::PROTECTED : ast::AccessModifier::PUBLIC)
                    );
                    if (isStatic) {
                        classDef->addStaticField(fieldMeta.name, fieldDef);
                    } else {
                        classDef->addInstanceField(fieldMeta.name, fieldDef);
                    }
                }
            };

            addFields(classMeta.instanceFields, false);
            addFields(classMeta.staticFields, true);

            // Add method stubs (no bytecode body — stubs only for type checking)
            auto addMethods = [&](const std::vector<vm::bytecode::BytecodeProgram::MethodMetadata>& methods, bool isStatic) {
                for (const auto& methodMeta : methods) {
                    // Build parameter list with correct ValueType for primitives
                    std::vector<std::pair<std::string, value::ValueType>> params;

                    // Instance methods need implicit 'this' parameter to match how
                    // ObjectClassBootstrap and the compiler register methods
                    if (!isStatic) {
                        params.emplace_back("this", value::ValueType::OBJECT);
                    }

                    for (size_t i = 0; i < methodMeta.parameterTypes.size(); ++i) {
                        std::string pName = (i < methodMeta.parameterNames.size())
                            ? methodMeta.parameterNames[i] : "p" + std::to_string(i);
                        params.emplace_back(pName, stringToValueType(methodMeta.parameterTypes[i]));
                    }

                    ast::AccessModifier access = methodMeta.isPrivate ? ast::AccessModifier::PRIVATE
                        : (methodMeta.isProtected ? ast::AccessModifier::PROTECTED : ast::AccessModifier::PUBLIC);

                    auto methodDef = std::make_shared<MethodDefinition>(
                        methodMeta.name,
                        stringToValueType(methodMeta.returnType),
                        params,
                        nullptr,  // no body — stub only
                        isStatic,
                        access
                    );
                    methodDef->setFinal(methodMeta.isFinal);
                    methodDef->setAbstract(methodMeta.isAbstract);

                    classDef->addMethod(methodDef);
                }
            };

            addMethods(classMeta.instanceMethods, false);
            addMethods(classMeta.staticMethods, true);

            // Add constructor stubs
            for (const auto& ctorMeta : classMeta.constructors) {
                std::vector<std::pair<std::string, value::ParameterType>> params;
                for (size_t i = 0; i < ctorMeta.parameterTypes.size(); ++i) {
                    std::string pName = (i < ctorMeta.parameterNames.size())
                        ? ctorMeta.parameterNames[i] : "p" + std::to_string(i);
                    params.emplace_back(pName, stringToParameterType(ctorMeta.parameterTypes[i]));
                }

                auto ctorDef = std::make_shared<ConstructorDefinition>(params, nullptr);
                classDef->addConstructor(ctorDef);
            }

            // Resolve alias: register under alias name if provided
            auto aliasIt = aliases.find(classMeta.name);
            std::string registrationName = (aliasIt != aliases.end()) ? aliasIt->second : classMeta.name;

            classMap[classMeta.name] = classDef;
            classRegistry->registerClass(registrationName, classDef);
        }

        // === Pass 2: Link parent classes ===
        for (const auto& classMeta : program.getClasses()) {
            if (classMeta.parentClassName.empty()) continue;

            auto it = classMap.find(classMeta.name);
            if (it == classMap.end()) continue;

            auto& classDef = it->second;

            // Try library-internal parent first
            auto parentIt = classMap.find(classMeta.parentClassName);
            if (parentIt != classMap.end()) {
                classDef->setParentClass(parentIt->second);
            } else {
                // Fall back to environment (e.g., Object, Exception from stdlib)
                auto parentDef = classRegistry->findClass(classMeta.parentClassName);
                if (parentDef) {
                    classDef->setParentClass(parentDef);
                }
            }
        }
    }

    void LibrarySymbolProvider::registerInterfaceStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const std::unordered_set<std::string>& filter,
        const std::unordered_map<std::string, std::string>& aliases)
    {
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) return;

        for (const auto& ifaceMeta : program.getInterfaces()) {
            if (!filter.empty() && filter.find(ifaceMeta.name) == filter.end()) continue;
            if (interfaceRegistry->hasInterface(ifaceMeta.name)) continue;

            std::vector<ast::GenericTypeParameter> genericParams;
            for (const auto& paramName : ifaceMeta.genericParameters) {
                genericParams.push_back(ast::GenericTypeParameter(paramName));
            }

            auto interfaceDef = std::make_shared<runtimeTypes::klass::InterfaceDefinition>(
                ifaceMeta.name, genericParams, ifaceMeta.extendsInterfaces);
            interfaceDef->setFinal(ifaceMeta.isFinal);

            for (const auto& methodMeta : ifaceMeta.methods) {
                runtimeTypes::klass::MethodSignature sig;
                sig.name = methodMeta.name;
                sig.returnType = ::types::UnifiedType::classType(methodMeta.returnType);

                for (size_t i = 0; i < methodMeta.parameterTypes.size(); ++i) {
                    std::string pName = (i < methodMeta.parameterNames.size())
                        ? methodMeta.parameterNames[i] : "p" + std::to_string(i);
                    sig.parameters.emplace_back(pName, ::types::UnifiedType::classType(methodMeta.parameterTypes[i]));
                }

                for (const auto& gp : methodMeta.genericTypeParameters) {
                    sig.genericParameters.push_back(ast::GenericTypeParameter(gp));
                }

                interfaceDef->addMethodSignature(sig);
            }

            auto ifaceAliasIt = aliases.find(ifaceMeta.name);
            std::string ifaceRegName = (ifaceAliasIt != aliases.end()) ? ifaceAliasIt->second : ifaceMeta.name;
            interfaceRegistry->registerInterface(ifaceRegName, interfaceDef);
        }
    }

    void LibrarySymbolProvider::registerFunctionStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const std::unordered_set<std::string>& filter,
        const std::unordered_map<std::string, std::string>& aliases)
    {
        auto functionRegistry = environment->getFunctionRegistry();
        if (!functionRegistry) return;

        for (const auto& [name, funcMeta] : program.getFunctions()) {
            // Skip internal/compiler-generated functions
            if (name.find("__") == 0) continue;
            // Skip if not in filter (when filter is active)
            if (!filter.empty() && filter.find(name) == filter.end()) continue;
            // Skip if already registered
            if (functionRegistry->hasFunction(name)) continue;

            // Build parameter list with correct ParameterType for primitives
            std::vector<std::pair<std::string, value::ParameterType>> params;
            for (size_t i = 0; i < funcMeta.parameterTypes.size(); ++i) {
                std::string pName = (i < funcMeta.parameterNames.size())
                    ? funcMeta.parameterNames[i] : "p" + std::to_string(i);
                params.emplace_back(pName, stringToParameterType(funcMeta.parameterTypes[i]));
            }

            // Create FunctionDefinition stub
            auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                name, stringToValueType(funcMeta.returnType), funcMeta.returnType, params);
            funcDef->setIsAsync(funcMeta.isAsync);

            // Set generic type parameters
            if (!funcMeta.genericTypeParameters.empty()) {
                std::vector<ast::GenericTypeParameter> genParams;
                for (const auto& gtp : funcMeta.genericTypeParameters) {
                    genParams.push_back(ast::GenericTypeParameter(gtp));
                }
                funcDef->setGenericTypeParameters(genParams);
            }

            auto funcAliasIt = aliases.find(name);
            std::string funcRegName = (funcAliasIt != aliases.end()) ? funcAliasIt->second : name;
            functionRegistry->registerFunction(funcRegName, funcDef);
        }
    }
}
