#include "ObjectClassBootstrap.hpp"
#include "../../Environment.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../value/ParameterType.hpp"
#include "../../../ast/AccessModifier.hpp"

namespace environment::registry::builtin
{
    void ObjectClassBootstrap::registerObjectClass(std::shared_ptr<environment::Environment> environment)
    {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry)
        {
            return;
        }

        // Don't register if Object already exists
        if (classRegistry->findClass("Object"))
        {
            return;
        }

        // Create the root Object ClassDefinition
        auto objectClass = std::make_shared<runtimeTypes::klass::ClassDefinition>("Object");
        objectClass->setFinal(false);
        objectClass->setAbstract(false);
        objectClass->setValueClass(false);

        // toString() : string — returns "ClassName@hashCode"
        {
            std::vector<std::pair<std::string, value::ParameterType>> params;
            // 'this' parameter is added implicitly by the VM, not in the definition
            auto toStringMethod = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                "toString",
                value::ValueType::STRING,
                params,
                nullptr, // No AST body — handled natively in ObjectExecutor
                false,   // Not static
                ast::AccessModifier::PUBLIC
            );
            objectClass->addInstanceMethod("toString", toStringMethod);
        }

        // equals(Object other) : bool — content-based equality
        {
            std::vector<std::pair<std::string, value::ParameterType>> params;
            params.emplace_back("other", value::ParameterType::forClass("Object"));
            auto equalsMethod = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                "equals",
                value::ValueType::BOOL,
                params,
                nullptr, // No AST body — handled natively in ObjectExecutor
                false,   // Not static
                ast::AccessModifier::PUBLIC
            );
            objectClass->addInstanceMethod("equals", equalsMethod);
        }

        // hashCode() : int — content-based hash
        {
            std::vector<std::pair<std::string, value::ParameterType>> params;
            auto hashCodeMethod = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                "hashCode",
                value::ValueType::INT,
                params,
                nullptr, // No AST body — handled natively in ObjectExecutor
                false,   // Not static
                ast::AccessModifier::PUBLIC
            );
            objectClass->addInstanceMethod("hashCode", hashCodeMethod);
        }

        // Register Object in ClassRegistry
        classRegistry->registerClass("Object", objectClass);
    }
}
