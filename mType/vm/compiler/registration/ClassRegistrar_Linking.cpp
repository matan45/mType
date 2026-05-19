#include "ClassRegistrar.hpp"
#include "../BytecodeCompiler.hpp"
#include "../../../analysis/OverrideAnnotationChecker.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../errors/AbstractClassException.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../validation/AnnotationValidator.hpp"
#include <stdexcept>
#include <unordered_set>

namespace vm::compiler::registration
{
    void ClassRegistrar::linkSingleClass(ast::ClassNode* classNode)
    {
        std::string className = classNode->getClassName();
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        if (!classNode->hasParentClass()) {
            if (className == "Object") {
                auto classDef = classRegistry->findClass(className);
                if (classDef && !classDef->isAbstract()) {
                    auto unimplemented = classDef->getUnimplementedAbstractMethods();
                    if (!unimplemented.empty()) {
                        std::string methodList;
                        for (size_t i = 0; i < unimplemented.size(); ++i) {
                            if (i > 0) methodList += ", ";
                            methodList += unimplemented[i];
                        }
                        throw errors::AbstractClassException(
                            "Concrete class '" + className + "' cannot have abstract methods: " + methodList,
                            classNode->getLocation()
                        );
                    }
                }
                return;
            }

            // Implicit Object parent — link it up explicitly.
            auto classDef = classRegistry->findClass(className);
            auto parentDef = classRegistry->findClass("Object");
            if (classDef && parentDef) {
                classDef->setParentClass(parentDef);

                ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);
                validateMethodOverrides(classDef, parentDef, classNode);

                // MYT-50: run missing-@Override checker for implicit-Object parents.
                if (compiler_) {
                    auto warns = analysis::OverrideAnnotationChecker::check(*classDef);
                    for (auto& w : warns) {
                        compiler_->addWarning(std::move(w));
                    }
                }

                if (!classDef->isAbstract()) {
                    auto unimplemented = classDef->getUnimplementedAbstractMethods();
                    if (!unimplemented.empty()) {
                        std::string methodList;
                        for (size_t i = 0; i < unimplemented.size(); ++i) {
                            if (i > 0) methodList += ", ";
                            methodList += unimplemented[i];
                        }
                        throw errors::AbstractClassException(
                            "Concrete class '" + className + "' cannot have abstract methods: " + methodList,
                            classNode->getLocation()
                        );
                    }
                }
            }
            return;
        }

        std::string parentClassName = classNode->getParentClassName();

        // Strip generic args: "Container<T>" → "Container"
        size_t genericStart = parentClassName.find('<');
        std::string baseParentClassName = (genericStart != std::string::npos)
            ? parentClassName.substr(0, genericStart)
            : parentClassName;

        validateParentClassExists(baseParentClassName, classNode->getLocation());

        auto classDef = classRegistry->findClass(className);
        auto parentDef = classRegistry->findClass(baseParentClassName);

        if (classDef && parentDef) {
            // Safety net for cross-file import scenarios. Parser already
            // validates same-file circular inheritance.
            checkCircularInheritance(className, parentClassName, parentDef);

            classDef->setParentClass(parentDef);

            // Map parent's generic params to concrete args: parent<T>, child
            // declares "Processor<String>" → substitution T → String.
            if (genericStart != std::string::npos) {
                parseAndStoreTypeSubstitutions(classDef, parentClassName, parentDef);
            }

            validateInheritanceDepth(className, classNode->getLocation());

            ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);
            validateMethodOverrides(classDef, parentDef, classNode);

            // MYT-50: missing-@Override warnings; checker is read-only and
            // skips classes without a parent, so it's safe once linked.
            if (compiler_) {
                auto warns = analysis::OverrideAnnotationChecker::check(*classDef);
                for (auto& w : warns) {
                    compiler_->addWarning(std::move(w));
                }
            }

            if (!classDef->isAbstract()) {
                auto unimplemented = classDef->getUnimplementedAbstractMethods();
                if (!unimplemented.empty()) {
                    std::string methodList;
                    for (size_t i = 0; i < unimplemented.size(); ++i) {
                        if (i > 0) methodList += ", ";
                        methodList += unimplemented[i];
                    }
                    throw errors::AbstractClassException(
                        "Concrete class '" + className + "' must implement all abstract methods: " + methodList,
                        classNode->getLocation()
                    );
                }
            }
        }
    }

    void ClassRegistrar::checkCircularInheritance(
        const std::string& className,
        const std::string& parentClassName,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentDef
    ) const
    {
        std::unordered_set<std::string> visited;
        auto currentCheck = parentDef;
        int depth = 0;
        while (currentCheck) {
            std::string checkName = currentCheck->getName();
            if (depth++ > 100) {
                throw errors::RuntimeException(
                    "Circular inheritance loop detected (depth > 100): class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            if (visited.count(checkName)) {
                throw errors::RuntimeException(
                    "Circular inheritance detected: class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            if (checkName == className) {
                throw errors::RuntimeException(
                    "Circular inheritance detected: class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            visited.insert(checkName);
            currentCheck = currentCheck->getParentClass();
        }
    }

    void ClassRegistrar::validateParentClassExists(
        const std::string& parentClassName,
        const ast::SourceLocation& location
    ) const
    {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        auto parentClass = classRegistry->findClass(parentClassName);
        if (!parentClass) {
            throw errors::InheritanceException(
                "Parent class '" + parentClassName + "' does not exist",
                location
            );
        }
    }

    void ClassRegistrar::validateInheritanceDepth(
        const std::string& className,
        const ast::SourceLocation& location
    ) const
    {
        inheritanceValidator->validateInheritanceDepth(className, location);
    }
}
