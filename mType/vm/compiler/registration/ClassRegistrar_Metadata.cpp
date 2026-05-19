#include "ClassRegistrar.hpp"
#include "AnnotationRetention.hpp"
#include "../../MethodSignature.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include "../../../types/TypeConversionBridge.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../environment/registry/MethodDefinition.hpp"

namespace vm::compiler::registration
{
    bytecode::BytecodeProgram::ClassMetadata ClassRegistrar::extractClassMetadata(ast::ClassNode* classNode) const
    {
        bytecode::BytecodeProgram::ClassMetadata metadata;

        metadata.name = classNode->getClassName();
        metadata.parentClassName = classNode->hasParentClass() ? classNode->getParentClassName() : "";
        metadata.isAbstract = classNode->isAbstract();
        metadata.isFinal = classNode->isFinal();
        metadata.isValueClass = classNode->isValueClass();

        for (const auto& iface : classNode->getImplementedInterfaces()) {
            metadata.implementedInterfaces.push_back(iface);
        }

        for (const auto& param : classNode->getGenericParameters()) {
            metadata.genericParameters.push_back(param.name);
        }

        // MYT-108: typed-arg annotations on class. MYT-109 strips SOURCE-
        // retention here so they don't reach the bytecode/runtime layers.
        for (const auto& annotationNode : classNode->getAnnotations()) {
            if (!annotationNode) continue;
            if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
            bytecode::BytecodeProgram::AnnotationData annot;
            populateAnnotationData(annot, *annotationNode);
            metadata.annotations.push_back(std::move(annot));
        }

        for (const auto& fieldNode : classNode->getFields()) {
            auto* field = dynamic_cast<ast::FieldNode*>(fieldNode.get());
            if (!field) continue;

            bytecode::BytecodeProgram::FieldMetadata fieldMeta;
            fieldMeta.name = field->getName();
            fieldMeta.type = ::types::TypeConversionUtils::getTypeDisplayName(field->getType());
            fieldMeta.isStatic = field->getIsStatic();
            fieldMeta.isFinal = field->getIsFinal();

            auto accessMod = field->getAccessModifier();
            fieldMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            fieldMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);

            // MYT-108 / MYT-109: typed-arg annotations, SOURCE-retention stripped.
            for (const auto& annotationNode : field->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                fieldMeta.annotations.push_back(std::move(annot));
            }

            if (field->getIsStatic()) {
                metadata.staticFields.push_back(fieldMeta);
            } else {
                metadata.instanceFields.push_back(fieldMeta);
            }
        }

        for (const auto& methodNode : classNode->getMethods()) {
            auto* method = dynamic_cast<ast::MethodNode*>(methodNode.get());
            if (!method) continue;

            bytecode::BytecodeProgram::MethodMetadata methodMeta;
            methodMeta.name = method->getName();
            methodMeta.returnType = ::types::TypeConversionUtils::getTypeDisplayName(method->getReturnType());
            methodMeta.isStatic = method->getIsStatic();
            methodMeta.isFinal = method->isFinal();

            auto accessMod = method->getAccessModifier();
            methodMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            methodMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);
            methodMeta.isAbstract = method->isAbstract();
            methodMeta.startOffset = 0;

            const auto& genericParams = method->getGenericParameters();
            for (const auto& [paramName, genType] : genericParams) {
                value::ValueType vType = value::ValueType::VOID;
                if (genType) {
                    auto uType = ::types::TypeConversionBridge::toUnifiedType(genType);
                    vType = uType->isGenericParameter() ? value::ValueType::OBJECT : uType->toValueType();
                }
                methodMeta.parameterTypes.push_back(::types::TypeConversionUtils::getTypeDisplayName(vType));
                methodMeta.parameterNames.push_back(paramName);
            }

            // MYT-108 / MYT-109: method annotations.
            for (const auto& annotationNode : method->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                methodMeta.annotations.push_back(std::move(annot));
            }

            // MYT-110: per-parameter annotations, parallel to AST parameters.
            const auto& methodParamAnnots = method->getParameterAnnotations();
            methodMeta.parameterAnnotations.resize(genericParams.size());
            for (size_t pi = 0; pi < genericParams.size(); ++pi) {
                if (pi >= methodParamAnnots.size()) continue;
                for (const auto& annotationNode : methodParamAnnots[pi]) {
                    if (!annotationNode) continue;
                    if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                    bytecode::BytecodeProgram::AnnotationData annot;
                    populateAnnotationData(annot, *annotationNode);
                    methodMeta.parameterAnnotations[pi].push_back(std::move(annot));
                }
            }

            if (method->getIsStatic()) {
                metadata.staticMethods.push_back(methodMeta);
            } else {
                metadata.instanceMethods.push_back(methodMeta);
            }

            // MYT-279: pre-register non-generic STATIC methods as callable
            // functions so caller-side type inference (findOverloadMetadata)
            // can resolve same-file Class::staticMethod() forward references.
            //
            // Skipped intentionally:
            //   * Instance methods — this.method() resolves via instanceMethods
            //     above; shadowing with a placeholder would break generic
            //     return inference (e.g. T[] toArray()).
            //   * Generic static methods — setupGenericTypeBindings would
            //     treat the placeholder as authoritative; mirroring
            //     FunctionRegistrar's type-parameter-usage population here is
            //     non-trivial. Generic forward refs are a follow-up.
            //
            // Shape mirrors collectMethodParameters so MethodCompilerHelper's
            // later overwrite (same qualified name) is byte-compatible.
            if (method->getIsStatic() && !method->isGeneric())
            {
                const std::string ownerClassName = classNode->getClassName();

                std::vector<std::string> fmParamNames;
                std::vector<std::string> fmParamTypes;
                std::vector<bool>        fmParamNullable;

                const auto& methodGenericParams = method->getGenericParameters();
                for (const auto& [pname, ptype] : methodGenericParams)
                {
                    fmParamNames.push_back(pname);
                    std::string ts = ptype ? ptype->toString() : "void";
                    bool isNullable = ptype && ptype->isNullable();
                    ts = ::types::TypeConversionUtils::stripNullable(ts);
                    fmParamTypes.push_back(ts);
                    fmParamNullable.push_back(isNullable);
                }

                std::string fmReturnType;
                if (auto genReturn = method->getGenericReturnType())
                {
                    fmReturnType = genReturn->toString();
                }
                else
                {
                    fmReturnType = ::types::TypeConversionUtils::getTypeDisplayName(method->getReturnType());
                }

                std::string qualifiedName = vm::MethodSignature(method->getName(), fmParamTypes)
                    .toMangledName(ownerClassName, /*isStatic=*/true);
                // Bare "Class::method" entry: callers parse `A::names()` into a
                // FunctionCallNode whose name is "A::names" (no $static, no
                // /sig). findOverloadMetadata's exact-match lookup uses that
                // raw name; without this entry zero-param static calls can't
                // resolve. Mirrors FunctionRegistrar registering both the bare
                // funcName and mangledName for free functions.
                const std::string bareQualifiedName = ownerClassName + "::" + method->getName();

                bytecode::BytecodeProgram::FunctionMetadata fm;
                fm.name              = qualifiedName;
                fm.startOffset       = 0;
                fm.instructionCount  = 0;
                fm.localCount        = 0;
                fm.parameterCount    = fmParamNames.size();
                fm.parameterNames    = fmParamNames;
                fm.parameterTypes    = fmParamTypes;
                fm.parameterNullable = fmParamNullable;
                fm.returnType        = fmReturnType;
                fm.isStatic          = true;
                fm.isNative          = false;
                fm.isAsync           = method->getIsAsync();
                program.registerFunction(qualifiedName, fm);
                program.registerFunction(bareQualifiedName, fm);
            }
        }

        for (const auto& ctorNode : classNode->getConstructors()) {
            auto* ctor = dynamic_cast<ast::ConstructorNode*>(ctorNode.get());
            if (!ctor) continue;

            bytecode::BytecodeProgram::ConstructorMetadata ctorMeta;
            ctorMeta.startOffset = 0;

            const auto& params = ctor->getParametersWithTypes();
            for (const auto& param : params) {
                ctorMeta.parameterNames.push_back(param.first);
                ctorMeta.parameterTypes.push_back(param.second.toString());
            }

            // MYT-108 / MYT-109: ctor annotations.
            for (const auto& annotationNode : ctor->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                ctorMeta.annotations.push_back(std::move(annot));
            }

            // MYT-110: per-parameter annotations.
            const auto& ctorParamAnnots = ctor->getParameterAnnotations();
            ctorMeta.parameterAnnotations.resize(params.size());
            for (size_t pi = 0; pi < params.size(); ++pi) {
                if (pi >= ctorParamAnnots.size()) continue;
                for (const auto& annotationNode : ctorParamAnnots[pi]) {
                    if (!annotationNode) continue;
                    if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                    bytecode::BytecodeProgram::AnnotationData annot;
                    populateAnnotationData(annot, *annotationNode);
                    ctorMeta.parameterAnnotations[pi].push_back(std::move(annot));
                }
            }

            metadata.constructors.push_back(ctorMeta);
        }

        return metadata;
    }

    std::vector<std::string> ClassRegistrar::parseGenericTypeArguments(const std::string& classNameWithGenerics) const
    {
        std::vector<std::string> typeArgs;

        size_t start = classNameWithGenerics.find('<');
        if (start == std::string::npos) {
            return typeArgs;
        }

        size_t end = classNameWithGenerics.rfind('>');
        if (end == std::string::npos || end <= start) {
            return typeArgs;
        }

        std::string argsStr = classNameWithGenerics.substr(start + 1, end - start - 1);

        // Top-level comma split, respecting nested angle brackets.
        std::string current;
        int depth = 0;

        for (char c : argsStr) {
            if (c == '<') {
                depth++;
                current += c;
            } else if (c == '>') {
                depth--;
                current += c;
            } else if (c == ',' && depth == 0) {
                size_t firstNonSpace = current.find_first_not_of(" \t");
                size_t lastNonSpace = current.find_last_not_of(" \t");
                if (firstNonSpace != std::string::npos) {
                    typeArgs.push_back(current.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
                }
                current.clear();
            } else if (c != ' ' && c != '\t') {
                if (!current.empty() || (c != ' ' && c != '\t')) {
                    current += c;
                }
            } else if (!current.empty()) {
                current += c;
            }
        }

        size_t firstNonSpace = current.find_first_not_of(" \t");
        size_t lastNonSpace = current.find_last_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            typeArgs.push_back(current.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
        }

        return typeArgs;
    }

    void ClassRegistrar::parseAndStoreTypeSubstitutions(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        const std::string& parentClassNameWithGenerics,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
    ) const
    {
        // "Processor<String>" → ["String"]
        auto typeArgs = parseGenericTypeArguments(parentClassNameWithGenerics);

        // Processor<T> → ["T"]
        const auto& parentGenericParams = parentClass->getGenericParameters();

        std::unordered_map<std::string, std::string> substitutionMap;
        size_t numParams = std::min(typeArgs.size(), parentGenericParams.size());
        for (size_t i = 0; i < numParams; ++i) {
            substitutionMap[parentGenericParams[i].name] = typeArgs[i];
        }

        childClass->setParentTypeSubstitutionMap(substitutionMap);
    }

    void ClassRegistrar::parseAndStoreUnifiedTypeSubstitutions(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        const std::string& parentClassNameWithGenerics,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
    ) const
    {
        auto typeArgsStrings = parseGenericTypeArguments(parentClassNameWithGenerics);

        std::vector<::types::UnifiedTypePtr> typeArgs;
        for (const auto& argStr : typeArgsStrings) {
            typeArgs.push_back(::types::UnifiedType::classType(argStr));
        }

        const auto& parentGenericParams = parentClass->getGenericParameters();

        if (typeSubstitutionService) {
            try {
                auto unifiedSubstitutionMap = typeSubstitutionService->buildSubstitutionMap(
                    parentGenericParams,
                    typeArgs
                );

                std::unordered_map<std::string, std::string> stringSubstitutionMap;
                for (const auto& [paramName, typePtr] : unifiedSubstitutionMap) {
                    if (typePtr) {
                        stringSubstitutionMap[paramName] = typePtr->toString();
                    }
                }

                childClass->setParentTypeSubstitutionMap(stringSubstitutionMap);
            } catch (const std::exception&) {
                parseAndStoreTypeSubstitutions(childClass, parentClassNameWithGenerics, parentClass);
            }
        } else {
            parseAndStoreTypeSubstitutions(childClass, parentClassNameWithGenerics, parentClass);
        }
    }
}
