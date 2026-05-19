#include "ClassRegistrar.hpp"
#include "AnnotationRetention.hpp"
#include "../../MethodSignature.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../errors/DuplicateDeclarationException.hpp"
#include "../../../errors/DuplicateSignatureException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../types/TypeConversionBridge.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../environment/registry/MethodDefinition.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../environment/registry/ConstructorDefinition.hpp"
#include "../../../environment/registry/FieldDefinition.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
    void ClassRegistrar::registerSingleClass(ast::ClassNode* classNode)
    {
        std::string className = classNode->getClassName();

        // Diamond-import vs redefinition: same physical node seen twice is
        // legitimate (A imports B, A imports C, both import D); a different
        // node with the same name is a real redefinition.
        auto seenIt = firstClassNodeByName.find(className);
        if (seenIt != firstClassNodeByName.end())
        {
            if (seenIt->second == classNode) {
                return;
            }
            throw errors::DuplicateDeclarationException(
                "class",
                className,
                seenIt->second->getLocation(),
                classNode->getLocation()
            );
        }

        // Built-in classes (e.g. Object) are pre-registered without entries in
        // firstClassNodeByName; preserve the original silent skip for those.
        if (environment->findClass(className)) {
            return;
        }

        firstClassNodeByName[className] = classNode;

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(className);
        classDef->setGenericParameters(classNode->getGenericParameters());
        classDef->setFinal(classNode->isFinal());
        classDef->setAbstract(classNode->isAbstract());
        classDef->setValueClass(classNode->isValueClass());

        for (const auto& annotation : classNode->getAnnotations()) {
            if (!annotation) continue;
            if (!shouldRetainAnnotation(*annotation, environment)) continue;
            classDef->addAnnotation(annotation);
        }

        if (classNode->hasParentClass()) {
            // Parser validates classes-extending-interfaces at parse time
            // (ClassDeclarationParser.cpp:92-98), so no check here.
            classDef->setParentClassName(classNode->getParentClassName());
        }
        else if (className != "Object") {
            classDef->setParentClassName("Object");
        }

        for (const auto& interfaceName : classNode->getImplementedInterfaces()) {
            classDef->addImplementedInterface(interfaceName);
        }

        registerSingleClassConstructors(classNode, classDef);
        registerSingleClassMethods(classNode, className, classDef);
        registerSingleClassFields(classNode, classDef);

        classRegistry->registerClass(className, classDef);

        if (interfaceRegistrar && !classDef->getImplementedInterfaces().empty()) {
            interfaceRegistrar->validateInterfaceImplementations(classDef, classNode);
        }

        auto classMetadata = extractClassMetadata(classNode);
        program.registerClass(classMetadata);
    }

    void ClassRegistrar::registerSingleClassConstructors(
        ast::ClassNode* classNode,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        if (classNode->getConstructors().empty()) {
            std::vector<std::pair<std::string, value::ParameterType>> emptyParams;
            auto defaultCtor = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                emptyParams,
                nullptr
            );
            classDef->addConstructor(defaultCtor);
            return;
        }

        for (const auto& constructor : classNode->getConstructors()) {
            auto* ctorNode = dynamic_cast<ast::nodes::classes::ConstructorNode*>(constructor.get());
            if (!ctorNode) continue;

            auto ctorDef = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                ctorNode->getParametersWithTypes(),
                ctorNode->getBody(),
                ctorNode->getAccessModifier()
            );

            // MYT-108: copy ctor annotations (drop SOURCE-retention per MYT-109).
            for (const auto& annotation : ctorNode->getAnnotations()) {
                if (!annotation) continue;
                if (!shouldRetainAnnotation(*annotation, environment)) continue;
                ctorDef->addAnnotation(annotation);
            }

            // MYT-110: per-parameter annotations (with retention filter); ctor
            // params are 1:1 between AST and runtime (no implicit `this`).
            const auto& astParamAnnots = ctorNode->getParameterAnnotations();
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> filtered;
            filtered.reserve(astParamAnnots.size());
            for (const auto& perParam : astParamAnnots)
            {
                std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> kept;
                for (const auto& a : perParam)
                {
                    if (!a) continue;
                    if (!shouldRetainAnnotation(*a, environment)) continue;
                    kept.push_back(a);
                }
                filtered.push_back(std::move(kept));
            }
            ctorDef->setParameterAnnotations(std::move(filtered));

            classDef->addConstructor(ctorDef);
        }
    }

    value::ParameterType ClassRegistrar::resolveMethodParameterType(
        const std::string& className,
        const ast::GenericType* genericType) const
    {
        if (!genericType) {
            return value::ParameterType(value::ValueType::VOID);
        }

        // Generic-parameter slot can mean: actual type param (T), a class or
        // interface name, a parameterized form (Container<Int>), or an array.
        // Use toString() to preserve generic args.
        if (genericType->isGenericParameter()) {
            std::string typeName = genericType->toString();

            std::string baseTypeName = typeName;
            size_t anglePos = typeName.find('<');
            if (anglePos != std::string::npos) {
                baseTypeName = typeName.substr(0, anglePos);
            }

            // Self-reference (e.g., String::equals(String other)) — bind to the
            // class being registered without consulting the registry.
            if (baseTypeName == className) {
                return value::ParameterType::forClass(typeName);
            }

            auto classRegistry = environment->getClassRegistry();
            auto interfaceRegistry = environment->getInterfaceRegistry();

            bool isClass = classRegistry && classRegistry->findClass(baseTypeName);
            bool isInterface = interfaceRegistry && interfaceRegistry->findInterface(baseTypeName);

            if (isClass) {
                return value::ParameterType::forClass(typeName);
            }
            if (isInterface) {
                return value::ParameterType::forInterface(typeName);
            }

            // MYT-282: array params carry ARRAY tag + precise form ("int[]",
            // "Animal[][]"). Pre-MYT-282 these were coerced to forClass()
            // and lied about basicType (claiming OBJECT).
            if (typeName.size() >= 2 &&
                typeName.compare(typeName.size() - 2, 2, "[]") == 0)
            {
                return value::ParameterType::forArray(typeName);
            }

            return value::ParameterType::forClass(typeName);
        }

        return value::ParameterType(genericType->getConcreteType());
    }

    void ClassRegistrar::registerSingleClassMethods(
        ast::ClassNode* classNode,
        const std::string& className,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        for (const auto& method : classNode->getMethods()) {
            auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get());
            if (!methodNode) continue;

            std::vector<std::pair<std::string, value::ParameterType>> params;

            if (!methodNode->getIsStatic()) {
                params.emplace_back("this", value::ParameterType::forClass(className));
            }

            for (const auto& [name, genericType] : methodNode->getGenericParameters()) {
                params.emplace_back(name, resolveMethodParameterType(className, genericType.get()));
            }

            auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                methodNode->getName(),
                methodNode->getReturnType(),
                params,
                methodNode->getBody(),
                methodNode->getIsStatic(),
                methodNode->getAccessModifier()
            );

            methodDef->setSourceLocation(methodNode->getLocation());

            if (methodNode->getGenericReturnType()) {
                methodDef->setUnifiedReturnType(
                    ::types::TypeConversionBridge::toUnifiedType(methodNode->getGenericReturnType()));
            }
            {
                std::vector<std::pair<std::string, ::types::UnifiedTypePtr>> convertedParams;
                for (const auto& [pName, genType] : methodNode->getGenericParameters()) {
                    convertedParams.emplace_back(pName,
                        ::types::TypeConversionBridge::toUnifiedType(genType));
                }
                methodDef->setUnifiedParameters(convertedParams);
            }
            methodDef->setGenericTypeParameters(methodNode->getGenericTypeParameters());

            methodDef->setAbstract(methodNode->isAbstract());
            if (methodNode->isAbstract()) {
                classDef->addAbstractMethod(methodNode->getName());
            }

            methodDef->setFinal(methodNode->isFinal());

            // MYT-113: propagate async flag so VM::invokeMethod takes the
            // Promise-returning interop path for async targets.
            methodDef->setIsAsync(methodNode->getIsAsync());

            // MYT-274: propagate synthetic flag so reflection filters
            // compiler-generated hashCode/equals out of getDeclaredMethods().
            methodDef->setSynthetic(methodNode->isSynthetic());

            for (const auto& annotation : methodNode->getAnnotations()) {
                if (!annotation) continue;
                if (!shouldRetainAnnotation(*annotation, environment)) continue;
                methodDef->addAnnotation(annotation);
            }

            // MYT-110: per-parameter annotations. Instance methods get an
            // implicit `this` slot at runtime-param 0 with no AST annotations.
            {
                const auto& astParamAnnots = methodNode->getParameterAnnotations();
                std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> filtered;
                const size_t runtimeParamCount = methodDef->getParameters().size();
                filtered.reserve(runtimeParamCount);
                const bool hasThis = !methodNode->getIsStatic();
                if (hasThis)
                {
                    filtered.push_back({});
                }
                for (const auto& perParam : astParamAnnots)
                {
                    std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> kept;
                    for (const auto& a : perParam)
                    {
                        if (!a) continue;
                        if (!shouldRetainAnnotation(*a, environment)) continue;
                        kept.push_back(a);
                    }
                    filtered.push_back(std::move(kept));
                }
                methodDef->setParameterAnnotations(std::move(filtered));
            }

            if (methodNode->getIsStatic()) {
                auto existingMethod = classDef->findStaticMethodBySignature(
                    methodNode->getName(),
                    methodDef->getParameters()
                );
                if (existingMethod) {
                    throw errors::DuplicateSignatureException(
                        "static method",
                        methodNode->getName(),
                        runtimeTypes::klass::SignatureUtils::generateTypeSignature(methodDef->getParameters()),
                        existingMethod->getSourceLocation(),
                        methodNode->getLocation()
                    );
                }
                classDef->addStaticMethod(methodNode->getName(), methodDef);
            } else {
                auto existingMethod = classDef->findInstanceMethodBySignature(
                    methodNode->getName(),
                    methodDef->getParameters()
                );
                if (existingMethod) {
                    auto paramsWithoutThis = methodDef->getParameters();
                    if (!paramsWithoutThis.empty() && paramsWithoutThis[0].first == "this") {
                        paramsWithoutThis.erase(paramsWithoutThis.begin());
                    }

                    throw errors::DuplicateSignatureException(
                        "method",
                        methodNode->getName(),
                        runtimeTypes::klass::SignatureUtils::generateTypeSignature(paramsWithoutThis),
                        existingMethod->getSourceLocation(),
                        methodNode->getLocation()
                    );
                }
                classDef->addMethod(methodDef);
            }
        }
    }

    void ClassRegistrar::registerSingleClassFields(
        ast::ClassNode* classNode,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        for (const auto& field : classNode->getFields()) {
            auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get());
            if (!fieldNode) continue;

            value::Value defaultValue;
            if (fieldNode->getIsStatic())
            {
                value::ValueType fieldType = fieldNode->getType();
                switch (fieldType)
                {
                case value::ValueType::INT:    defaultValue = 0; break;
                case value::ValueType::FLOAT:  defaultValue = 0.0; break;
                case value::ValueType::STRING: defaultValue = std::string(""); break;
                case value::ValueType::BOOL:   defaultValue = false; break;
                default:                       defaultValue = nullptr; break;
                }
            }
            else
            {
                // Instance fields use monostate sentinel; ObjectInstanceHelper
                // applies the proper type default at allocation time.
                defaultValue = std::monostate{};
            }

            auto fieldDef = std::make_shared<runtimeTypes::klass::FieldDefinition>(
                fieldNode->getName(),
                fieldNode->getType(),
                ::types::TypeConversionBridge::toUnifiedType(fieldNode->getGenericType()),
                defaultValue,
                fieldNode->getIsStatic(),
                fieldNode->getIsFinal(),
                fieldNode->getAccessModifier()
            );

            // MYT-108: field annotations (drop SOURCE-retention per MYT-109).
            for (const auto& annotation : fieldNode->getAnnotations()) {
                if (!annotation) continue;
                if (!shouldRetainAnnotation(*annotation, environment)) continue;
                fieldDef->addAnnotation(annotation);
            }

            if (fieldNode->getIsStatic()) {
                classDef->addStaticField(fieldNode->getName(), fieldDef);
            } else {
                classDef->addInstanceField(fieldNode->getName(), fieldDef);
            }
        }
    }
}
