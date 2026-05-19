#include "BytecodeService.hpp"
#include <cstddef>
#include "../validation/AnnotationValidator.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/MethodDefinition.hpp"
#include "../environment/registry/FieldDefinition.hpp"
#include "../environment/registry/ConstructorDefinition.hpp"
#include "../environment/registry/InterfaceDefinition.hpp"
#include "../environment/registry/InterfaceRegistry.hpp"
#include "../types/UnifiedType.hpp"
#include "../ast/GenericType.hpp"
#include "../ast/nodes/annotations/AnnotationNode.hpp"

namespace
{
    value::ValueType stringToValueType(const std::string& typeName)
    {
        if (typeName == "int") return value::ValueType::INT;
        if (typeName == "float") return value::ValueType::FLOAT;
        if (typeName == "bool") return value::ValueType::BOOL;
        if (typeName == "string") return value::ValueType::STRING;
        if (typeName == "void") return value::ValueType::VOID;
        if (typeName == "null") return value::ValueType::NULL_TYPE;
        return value::ValueType::OBJECT;
    }
}

namespace services
{
    void BytecodeService::registerClassesFromMetadata(
        const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes)
    {
        using namespace runtimeTypes::klass;

        auto classRegistry = environment->getClassRegistry();

        // First pass: create all ClassDefinitions (skip already registered classes).
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classMap;
        createClassDefinitionsFirstPass(classes, classMap);

        // Second pass: link parent classes and populate members.
        for (const auto& classMeta : classes)
        {
            // Hot-reload of modified scripts: drop the prior class definition
            // before re-registering, so callers see the new shape.
            if (classRegistry->findClass(classMeta.name))
            {
                classRegistry->removeItem(classMeta.name);
            }

            auto classDef = classMap[classMeta.name];
            populateClassFromMetadata(classMeta, classDef, classMap);

            ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);

            classRegistry->registerClass(classMeta.name, classDef);
        }
    }

    std::shared_ptr<ast::nodes::annotations::AnnotationNode>
        BytecodeService::buildAnnotationNodeFromMetadata(
            const vm::bytecode::BytecodeProgram::AnnotationData& annotData)
    {
        using ast::nodes::annotations::AnnotationValueType;
        using ast::nodes::annotations::TypedAnnotationValue;

        auto node = std::make_shared<ast::nodes::annotations::AnnotationNode>(
            annotData.name, annotData.location);
        for (const auto& arg : annotData.typedArguments)
        {
            auto t = static_cast<AnnotationValueType>(arg.valueType);
            TypedAnnotationValue v;
            switch (t)
            {
            case AnnotationValueType::INT:         v = TypedAnnotationValue::makeInt(arg.intVal); break;
            case AnnotationValueType::FLOAT:       v = TypedAnnotationValue::makeFloat(arg.floatVal); break;
            case AnnotationValueType::BOOL:        v = TypedAnnotationValue::makeBool(arg.boolVal); break;
            case AnnotationValueType::STRING:      v = TypedAnnotationValue::makeString(arg.stringVal); break;
            case AnnotationValueType::CLASS_REF:   v = TypedAnnotationValue::makeClassRef(arg.stringVal); break;
            case AnnotationValueType::CLASS_ARRAY: v = TypedAnnotationValue::makeClassArray(arg.arrayVal); break;
            case AnnotationValueType::NULL_VALUE:  v = TypedAnnotationValue::makeNull(); break;
            }
            node->setTypedParameter(arg.key, std::move(v));
        }
        return node;
    }

    void BytecodeService::registerAnnotationsFromMetadata(
        const std::vector<vm::bytecode::BytecodeProgram::AnnotationDeclData>& declarations)
    {
        using ast::nodes::annotations::AnnotationValueType;
        using ast::nodes::annotations::TypedAnnotationValue;

        auto annotationRegistry = environment->getAnnotationRegistry();
        if (!annotationRegistry) return;

        for (const auto& decl : declarations)
        {
            if (annotationRegistry->hasAnnotation(decl.name)) continue;
            auto def = std::make_shared<runtimeTypes::klass::AnnotationDefinition>(decl.name, false);
            for (const auto& p : decl.params)
            {
                runtimeTypes::klass::AnnotationParamSchema schema;
                schema.name         = p.name;
                schema.declaredType = static_cast<AnnotationValueType>(p.declaredType);
                schema.nullable     = p.nullable;
                schema.isArray      = p.isArray;
                if (p.hasDefault)
                {
                    switch (schema.declaredType)
                    {
                    case AnnotationValueType::INT:         schema.defaultValue = TypedAnnotationValue::makeInt(p.defaultInt); break;
                    case AnnotationValueType::FLOAT:       schema.defaultValue = TypedAnnotationValue::makeFloat(p.defaultFloat); break;
                    case AnnotationValueType::BOOL:        schema.defaultValue = TypedAnnotationValue::makeBool(p.defaultBool); break;
                    case AnnotationValueType::STRING:      schema.defaultValue = TypedAnnotationValue::makeString(p.defaultString); break;
                    case AnnotationValueType::CLASS_REF:   schema.defaultValue = TypedAnnotationValue::makeClassRef(p.defaultString); break;
                    case AnnotationValueType::CLASS_ARRAY: schema.defaultValue = TypedAnnotationValue::makeClassArray(p.defaultStringArray); break;
                    case AnnotationValueType::NULL_VALUE:  schema.defaultValue = TypedAnnotationValue::makeNull(); break;
                    }
                }
                def->addParam(std::move(schema));
            }
            // MYT-109: rebuild meta-annotation AST nodes from serialized form
            // so reflection / @Retention / @Target enforcement work after load.
            for (const auto& metaData : decl.metaAnnotations)
            {
                def->addMetaAnnotation(buildAnnotationNodeFromMetadata(metaData));
            }
            annotationRegistry->registerAnnotation(decl.name, def);
        }
    }

    void BytecodeService::registerInterfacesFromMetadata(
        const std::vector<vm::bytecode::BytecodeProgram::InterfaceMetadata>& interfaces)
    {
        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry) return;

        for (const auto& ifaceMeta : interfaces)
        {
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
                    std::string paramName = (i < methodMeta.parameterNames.size())
                        ? methodMeta.parameterNames[i] : "p" + std::to_string(i);
                    auto paramType = ::types::UnifiedType::classType(methodMeta.parameterTypes[i]);
                    sig.parameters.emplace_back(paramName, paramType);
                }

                for (const auto& gp : methodMeta.genericTypeParameters) {
                    sig.genericParameters.push_back(ast::GenericTypeParameter(gp));
                }

                interfaceDef->addMethodSignature(sig);
            }

            interfaceRegistry->registerInterface(ifaceMeta.name, interfaceDef);
        }
    }

    void BytecodeService::createClassDefinitionsFirstPass(
        const std::vector<vm::bytecode::BytecodeProgram::ClassMetadata>& classes,
        std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap)
    {
        using namespace runtimeTypes::klass;

        for (const auto& classMeta : classes)
        {
            std::vector<ast::GenericTypeParameter> genericParams;
            for (const auto& paramName : classMeta.genericParameters)
            {
                genericParams.push_back(ast::GenericTypeParameter(paramName));
            }

            auto classDef = std::make_shared<ClassDefinition>(classMeta.name, genericParams);

            if (!classMeta.parentClassName.empty())
            {
                classDef->setParentClassName(classMeta.parentClassName);
            }
            classDef->setImplementedInterfaces(classMeta.implementedInterfaces);

            classDef->setAbstract(classMeta.isAbstract);
            classDef->setFinal(classMeta.isFinal);
            classDef->setValueClass(classMeta.isValueClass);

            // MYT-108: restore class annotations from bytecode metadata (typed-args).
            for (const auto& annotData : classMeta.annotations)
            {
                classDef->addAnnotation(buildAnnotationNodeFromMetadata(annotData));
            }

            classMap[classMeta.name] = classDef;
        }
    }

    void BytecodeService::populateClassFromMetadata(
        const vm::bytecode::BytecodeProgram::ClassMetadata& classMeta,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>>& classMap)
    {
        // Link parent class — check bytecode metadata first, then fall back to
        // the environment's class registry for library classes (e.g. Exception)
        // that may have been registered separately.
        if (!classMeta.parentClassName.empty())
        {
            if (classMap.count(classMeta.parentClassName))
            {
                classDef->setParentClass(classMap.at(classMeta.parentClassName));
            }
            else
            {
                auto classRegistry = environment->getClassRegistry();
                if (classRegistry)
                {
                    auto parentDef = classRegistry->findClass(classMeta.parentClassName);
                    if (parentDef)
                    {
                        classDef->setParentClass(parentDef);
                    }
                }
            }
        }

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

            // Static fields get a type-appropriate default; instance fields
            // are zeroed here and initialized in the constructor.
            value::Value defaultValue;
            if (isStatic)
            {
                switch (fieldType)
                {
                case value::ValueType::INT:
                    defaultValue = 0;
                    break;
                case value::ValueType::FLOAT:
                    defaultValue = 0.0;
                    break;
                case value::ValueType::STRING:
                    defaultValue = std::string("");
                    break;
                case value::ValueType::BOOL:
                    defaultValue = false;
                    break;
                default:
                    defaultValue = std::monostate{};
                    break;
                }
            }
            else
            {
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

            // MYT-108: restore field annotations from bytecode metadata.
            for (const auto& annotData : fieldMeta.annotations) {
                fieldDef->addAnnotation(buildAnnotationNodeFromMetadata(annotData));
            }

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

            // Instance methods need a 'this' parameter prepended so that
            // findInstanceMethod's argCount subtraction is consistent with
            // methods compiled from AST (which always include 'this').
            if (!isStatic)
            {
                params.push_back({"this", value::ParameterType(value::ValueType::OBJECT)});
            }

            for (size_t i = 0; i < methodMeta.parameterNames.size(); ++i)
            {
                auto paramType = stringToValueType(methodMeta.parameterTypes[i]);
                const std::string& typeStr = methodMeta.parameterTypes[i];

                // Preserve generic type parameters and class names for proper type signature matching.
                if (paramType == value::ValueType::OBJECT) {
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
                nullptr,
                isStatic,
                accessMod
            );

            methodDef->setAbstract(methodMeta.isAbstract);
            methodDef->setFinal(methodMeta.isFinal);

            if (methodMeta.isAbstract) {
                classDef->addAbstractMethod(methodMeta.name);
            }

            // MYT-108: restore method annotations from bytecode metadata.
            for (const auto& annotData : methodMeta.annotations) {
                methodDef->addAnnotation(buildAnnotationNodeFromMetadata(annotData));
            }

            // MYT-110: restore per-parameter annotations. methodDef's params
            // prepend `this` for instance methods; metadata does not carry
            // that slot, so we offset by one.
            {
                std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> restored;
                const size_t runtimeSize = methodDef->getParameters().size();
                restored.reserve(runtimeSize);
                if (!isStatic) restored.push_back({}); // `this`
                for (const auto& perParam : methodMeta.parameterAnnotations) {
                    std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> nodes;
                    nodes.reserve(perParam.size());
                    for (const auto& ad : perParam) {
                        nodes.push_back(buildAnnotationNodeFromMetadata(ad));
                    }
                    restored.push_back(std::move(nodes));
                }
                methodDef->setParameterAnnotations(std::move(restored));
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

                if (paramType == value::ValueType::OBJECT) {
                    value::ParameterType paramTypeWithClass(paramType);
                    paramTypeWithClass.className = typeStr;
                    params.push_back({ctorMeta.parameterNames[i], paramTypeWithClass});
                } else {
                    params.push_back({ctorMeta.parameterNames[i], value::ParameterType(paramType)});
                }
            }

            auto ctorDef = std::make_shared<ConstructorDefinition>(
                params,
                nullptr,
                ast::AccessModifier::PUBLIC
            );

            // MYT-108: restore constructor annotations from bytecode metadata.
            for (const auto& annotData : ctorMeta.annotations) {
                ctorDef->addAnnotation(buildAnnotationNodeFromMetadata(annotData));
            }

            // MYT-110: restore per-parameter annotations (1:1 with metadata).
            {
                std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> restored;
                restored.reserve(ctorMeta.parameterAnnotations.size());
                for (const auto& perParam : ctorMeta.parameterAnnotations) {
                    std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> nodes;
                    nodes.reserve(perParam.size());
                    for (const auto& ad : perParam) {
                        nodes.push_back(buildAnnotationNodeFromMetadata(ad));
                    }
                    restored.push_back(std::move(nodes));
                }
                ctorDef->setParameterAnnotations(std::move(restored));
            }
            classDef->addConstructor(ctorDef);
        }
    }
}
