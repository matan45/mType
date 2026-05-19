#include "AnnotationValidator.hpp"
#include <cstddef>
#include "AnnotationUsageValidator.hpp"
#include "../errors/InheritanceException.hpp"
#include "../errors/TypeException.hpp"
#include "../environment/registry/InterfaceDefinition.hpp"
#include "../environment/registry/InterfaceRegistry.hpp"
#include <sstream>
#include <unordered_set>

namespace validation
{
    void AnnotationValidator::validateClassAnnotations(
        std::shared_ptr<ClassDefinition> classDefinition,
        std::shared_ptr<Environment> environment)
    {
        if (!classDefinition || !environment)
        {
            return;
        }

        // First pass: run the typed usage validator on every annotation present
        // on this class and on every method. Catches unknown annotations,
        // wrong-type/missing/unknown params, and fills in defaults so the
        // built-in validators below see a complete value set.
        checkNoDuplicateAnnotations(classDefinition->getAnnotations(), "class");
        for (const auto& annotation : classDefinition->getAnnotations())
        {
            AnnotationUsageValidator::validate(
                annotation, environment, annotation->getLocation(),
                AnnotationHostKind::CLASS);
        }
        for (const auto& [methodName, methodOverloads] : classDefinition->getInstanceMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                checkNoDuplicateAnnotations(methodDef->getAnnotations(), "method");
                for (const auto& ann : methodDef->getAnnotations())
                {
                    AnnotationUsageValidator::validate(
                        ann, environment, ann->getLocation(),
                        AnnotationHostKind::METHOD);
                }
            }
        }
        for (const auto& [methodName, methodOverloads] : classDefinition->getStaticMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                checkNoDuplicateAnnotations(methodDef->getAnnotations(), "method");
                for (const auto& ann : methodDef->getAnnotations())
                {
                    AnnotationUsageValidator::validate(
                        ann, environment, ann->getLocation(),
                        AnnotationHostKind::METHOD);
                }
            }
        }
        for (const auto& ctorDef : classDefinition->getConstructors())
        {
            if (!ctorDef) continue;
            checkNoDuplicateAnnotations(ctorDef->getAnnotations(), "constructor");
            for (const auto& ann : ctorDef->getAnnotations())
            {
                AnnotationUsageValidator::validate(
                    ann, environment, ann->getLocation(),
                    AnnotationHostKind::CONSTRUCTOR);
            }
        }
        for (const auto& [fieldName, fieldDef] : classDefinition->getInstanceFields())
        {
            if (!fieldDef) continue;
            checkNoDuplicateAnnotations(fieldDef->getAnnotations(), "field");
            for (const auto& ann : fieldDef->getAnnotations())
            {
                AnnotationUsageValidator::validate(
                    ann, environment, ann->getLocation(),
                    AnnotationHostKind::FIELD);
            }
        }
        for (const auto& [fieldName, fieldDef] : classDefinition->getStaticFields())
        {
            if (!fieldDef) continue;
            checkNoDuplicateAnnotations(fieldDef->getAnnotations(), "field");
            for (const auto& ann : fieldDef->getAnnotations())
            {
                AnnotationUsageValidator::validate(
                    ann, environment, ann->getLocation(),
                    AnnotationHostKind::FIELD);
            }
        }

        // Parameter-level annotations: the usage validator checks @Target against HostKind::PARAMETER.
        auto validateParamAnnotations = [&](const auto& perParam)
        {
            checkNoDuplicateAnnotations(perParam, "parameter");
            for (const auto& ann : perParam)
            {
                if (!ann) continue;
                AnnotationUsageValidator::validate(
                    ann, environment, ann->getLocation(),
                    AnnotationHostKind::PARAMETER);
            }
        };
        for (const auto& [methodName, methodOverloads] : classDefinition->getInstanceMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                if (!methodDef) continue;
                for (const auto& perParam : methodDef->getParameterAnnotations())
                {
                    validateParamAnnotations(perParam);
                }
            }
        }
        for (const auto& [methodName, methodOverloads] : classDefinition->getStaticMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                if (!methodDef) continue;
                for (const auto& perParam : methodDef->getParameterAnnotations())
                {
                    validateParamAnnotations(perParam);
                }
            }
        }
        for (const auto& ctorDef : classDefinition->getConstructors())
        {
            if (!ctorDef) continue;
            for (const auto& perParam : ctorDef->getParameterAnnotations())
            {
                validateParamAnnotations(perParam);
            }
        }

        if (auto scriptAnnotation = classDefinition->getAnnotation("Script"))
        {
            SourceLocation location = scriptAnnotation->getLocation();
            validateScriptAnnotation(classDefinition, location);
        }

        if (auto entryPointAnnotation = classDefinition->getAnnotation("EntryPoint"))
        {
            SourceLocation location = entryPointAnnotation->getLocation();
            validateEntryPointAnnotation(classDefinition, location);
        }

        std::shared_ptr<ClassDefinition> parentClass = nullptr;
        if (classDefinition->hasParentClass())
        {
            auto classRegistry = environment->getClassRegistry();
            if (classRegistry)
            {
                parentClass = classRegistry->findClass(classDefinition->getParentClassName());
            }
        }

        for (const auto& [methodName, methodOverloads] : classDefinition->getInstanceMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                if (methodDef->hasAnnotation("Override"))
                {
                    validateOverrideAnnotation(
                        methodDef.get(),
                        classDefinition,
                        environment,
                        methodDef->getSourceLocation()
                    );
                }

                if (auto throwAnnotation = methodDef->getAnnotation("Throw"))
                {
                    validateThrowAnnotation(
                        throwAnnotation,
                        environment,
                        methodDef->getSourceLocation()
                    );
                }
            }
        }

        for (const auto& [methodName, methodOverloads] : classDefinition->getStaticMethods())
        {
            for (const auto& methodDef : methodOverloads)
            {
                if (auto throwAnnotation = methodDef->getAnnotation("Throw"))
                {
                    validateThrowAnnotation(
                        throwAnnotation,
                        environment,
                        methodDef->getSourceLocation()
                    );
                }
            }
        }
    }

    void AnnotationValidator::validateOverrideAnnotation(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> containingClass,
        std::shared_ptr<Environment> environment,
        const SourceLocation& location)
    {
        if (!method || !containingClass || !environment)
        {
            return;
        }

        bool foundMatchingMethod = false;

        if (containingClass->hasParentClass())
        {
            std::string fullParentName = containingClass->getParentClassName();

            // Strip generic type parameters: "Container<DataItem>" -> "Container"
            std::string baseParentName = fullParentName;
            size_t genericStart = fullParentName.find('<');
            if (genericStart != std::string::npos)
            {
                baseParentName = fullParentName.substr(0, genericStart);
            }

            auto classRegistry = environment->getClassRegistry();
            if (classRegistry)
            {
                auto parentClass = classRegistry->findClass(baseParentName);
                if (parentClass)
                {
                    auto parentMethod = findMatchingMethodInParent(method, parentClass);
                    if (parentMethod)
                    {
                        foundMatchingMethod = true;
                    }
                }
            }
        }

        if (!foundMatchingMethod)
        {
            foundMatchingMethod = findMatchingMethodInInterfaces(
                method,
                containingClass,
                environment
            );
        }

        if (!foundMatchingMethod)
        {
            std::string errorMsg = generateOverrideErrorMessage(
                method->getName(),
                containingClass->getName()
            );
            throw InheritanceException(errorMsg, location);
        }
    }

    std::shared_ptr<MethodDefinition> AnnotationValidator::findMatchingMethodInParent(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> parentClass)
    {
        if (!method || !parentClass)
        {
            return nullptr;
        }

        while (parentClass)
        {
            const auto& instanceMethods = parentClass->getInstanceMethods();

            auto it = instanceMethods.find(method->getName());
            if (it != instanceMethods.end())
            {
                for (const auto& parentMethod : it->second)
                {
                    if (methodSignaturesMatch(method, parentMethod.get()))
                    {
                        return parentMethod;
                    }
                }
            }

            parentClass = parentClass->getParentClass();
        }

        return nullptr;
    }

    bool AnnotationValidator::findMatchingMethodInInterfaces(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> containingClass,
        std::shared_ptr<Environment> environment)
    {
        if (!method || !containingClass || !environment)
        {
            return false;
        }

        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry)
        {
            return false;
        }

        // For instance methods, exclude implicit 'this' before comparing with interface methods.
        size_t methodParamCount = method->getParameters().size();
        if (!method->isStatic() && methodParamCount > 0)
        {
            methodParamCount--;
        }

        for (const std::string& interfaceName : containingClass->getImplementedInterfaces())
        {
            auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
            if (interfaceDef)
            {
                auto interfaceMethod = interfaceDef->findMethod(method->getName(), methodParamCount);
                if (interfaceMethod)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool AnnotationValidator::methodSignaturesMatch(
        const MethodDefinition* method1,
        const MethodDefinition* method2)
    {
        if (!method1 || !method2)
        {
            return false;
        }

        if (method1->getName() != method2->getName())
        {
            return false;
        }

        const auto& params1 = method1->getParameters();
        const auto& params2 = method2->getParameters();

        if (params1.size() != params2.size())
        {
            return false;
        }

        // Skip index 0 ('this') for instance methods — it differs between parent and child.
        size_t startIndex = (!method1->isStatic() && !params1.empty()) ? 1 : 0;
        for (size_t i = startIndex; i < params1.size(); ++i)
        {
            if (!(params1[i].second == params2[i].second))
            {
                return false;
            }
        }

        if (method1->getReturnType() != method2->getReturnType())
        {
            return false;
        }

        return true;
    }

    std::string AnnotationValidator::generateOverrideErrorMessage(
        const std::string& methodName,
        const std::string& className)
    {
        std::ostringstream oss;
        oss << "Method '" << methodName << "' in class '" << className
            << "' is marked with @Override but does not override any method from a parent class or interface.\n\n"
            << "Possible causes:\n"
            << "  - The method name may be misspelled\n"
            << "  - The parameter types may not match the parent method\n"
            << "  - The parent class does not have this method\n"
            << "  - The class does not implement an interface with this method\n\n"
            << "Please verify the method signature matches exactly with the parent method or interface.";
        return oss.str();
    }

    void AnnotationValidator::checkNoDuplicateAnnotations(
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>& annotations,
        const char* hostKindLabel)
    {
        std::unordered_set<std::string> seen;
        for (const auto& ann : annotations)
        {
            if (!ann) continue;
            const std::string& name = ann->getName();
            if (!seen.insert(name).second)
            {
                std::ostringstream oss;
                oss << "Duplicate annotation '@" << name << "' on " << hostKindLabel
                    << " — the same annotation cannot be applied more than once.";
                throw TypeException(oss.str(), ann->getLocation());
            }
        }
    }
}
