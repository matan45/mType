#include "LibrarySymbolProvider.hpp"
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
    void LibrarySymbolProvider::registerLibrarySymbols(
        const MtcLibProgram& library,
        std::shared_ptr<environment::Environment> environment)
    {
        const auto& program = library.bytecodeProgram;

        // Register in order: interfaces first (classes may implement them)
        registerInterfaceStubs(program, environment);
        registerClassStubs(program, environment);
        registerFunctionStubs(program, environment);
    }

    void LibrarySymbolProvider::registerClassStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment)
    {
        using namespace runtimeTypes::klass;

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) return;

        for (const auto& classMeta : program.getClasses()) {
            // Skip if already registered
            if (classRegistry->findClass(classMeta.name)) continue;

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
                    // Build parameter list as ValueType pairs
                    std::vector<std::pair<std::string, value::ValueType>> params;
                    for (size_t i = 0; i < methodMeta.parameterTypes.size(); ++i) {
                        std::string pName = (i < methodMeta.parameterNames.size())
                            ? methodMeta.parameterNames[i] : "p" + std::to_string(i);
                        params.emplace_back(pName, value::ValueType::OBJECT);
                    }

                    ast::AccessModifier access = methodMeta.isPrivate ? ast::AccessModifier::PRIVATE
                        : (methodMeta.isProtected ? ast::AccessModifier::PROTECTED : ast::AccessModifier::PUBLIC);

                    auto methodDef = std::make_shared<MethodDefinition>(
                        methodMeta.name,
                        value::ValueType::OBJECT,
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
                    params.emplace_back(pName, value::ParameterType::forClass(ctorMeta.parameterTypes[i]));
                }

                auto ctorDef = std::make_shared<ConstructorDefinition>(params, nullptr);
                classDef->addConstructor(ctorDef);
            }

            classRegistry->registerClass(classMeta.name, classDef);
        }
    }

    void LibrarySymbolProvider::registerInterfaceStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment)
    {
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) return;

        for (const auto& ifaceMeta : program.getInterfaces()) {
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

            interfaceRegistry->registerInterface(ifaceMeta.name, interfaceDef);
        }
    }

    void LibrarySymbolProvider::registerFunctionStubs(
        const vm::bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment)
    {
        auto functionRegistry = environment->getFunctionRegistry();
        if (!functionRegistry) return;

        for (const auto& [name, funcMeta] : program.getFunctions()) {
            // Skip internal/compiler-generated functions
            if (name.find("__") == 0) continue;

            // Skip if already registered
            if (functionRegistry->hasFunction(name)) continue;

            // Build parameter list with ParameterType
            std::vector<std::pair<std::string, value::ParameterType>> params;
            for (size_t i = 0; i < funcMeta.parameterTypes.size(); ++i) {
                std::string pName = (i < funcMeta.parameterNames.size())
                    ? funcMeta.parameterNames[i] : "p" + std::to_string(i);
                params.emplace_back(pName, value::ParameterType::forClass(funcMeta.parameterTypes[i]));
            }

            // Create FunctionDefinition stub
            auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                name, value::ValueType::OBJECT, funcMeta.returnType, params);
            funcDef->setIsAsync(funcMeta.isAsync);

            // Set generic type parameters
            if (!funcMeta.genericTypeParameters.empty()) {
                std::vector<ast::GenericTypeParameter> genParams;
                for (const auto& gtp : funcMeta.genericTypeParameters) {
                    genParams.push_back(ast::GenericTypeParameter(gtp));
                }
                funcDef->setGenericTypeParameters(genParams);
            }

            functionRegistry->registerFunction(name, funcDef);
        }
    }
}
