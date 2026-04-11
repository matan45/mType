#include "ObjectClassBootstrap.hpp"
#include "../../Environment.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../value/ParameterType.hpp"
#include "../../../ast/AccessModifier.hpp"
#include <stdexcept>

namespace environment::registry::builtin
{
    void registerObjectClass(const std::shared_ptr<environment::Environment>& environment)
    {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry)
        {
            throw std::runtime_error("Cannot register Object class: ClassRegistry is not available");
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
            // Include 'this' parameter to match how user-defined methods are stored
            params.emplace_back("this", value::ParameterType::forClass("Object"));
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
            params.emplace_back("this", value::ParameterType::forClass("Object"));
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
            params.emplace_back("this", value::ParameterType::forClass("Object"));
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

        // getClass() : Class — returns the runtime Class of this instance,
        // including parameterized type arguments for generic instances.
        // Implemented natively in ObjectExecutor — routes through
        // ScriptAPI::getClass which calls Class.forName, so there is a
        // single code path shared with the C++ FFI surface (MYT-42).
        {
            std::vector<std::pair<std::string, value::ParameterType>> params;
            params.emplace_back("this", value::ParameterType::forClass("Object"));
            auto getClassMethod = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                "getClass",
                value::ValueType::OBJECT,
                params,
                nullptr, // No AST body — handled natively in ObjectExecutor
                false,   // Not static
                ast::AccessModifier::PUBLIC
            );
            objectClass->addInstanceMethod("getClass", getClassMethod);
        }

        // Register Object in ClassRegistry
        classRegistry->registerClass("Object", objectClass);
    }
}
