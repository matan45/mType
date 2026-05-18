#pragma once

#include "Registry.hpp"
#include "AnnotationDefinition.hpp"

namespace environment::registry
{
    /// Registry of user-defined and built-in annotation type declarations.
    /// Mirrors ClassRegistry / InterfaceRegistry usage; built-ins are
    /// pre-registered at Environment::initialize() via BuiltInAnnotations.
    class AnnotationRegistry : public Registry<runtimeTypes::klass::AnnotationDefinition>
    {
    public:
        std::string getComponentName() const override { return "AnnotationRegistry"; }

        void registerAnnotation(const std::string& name,
                                std::shared_ptr<runtimeTypes::klass::AnnotationDefinition> def)
        {
            registerItem(name, std::move(def));
        }

        std::shared_ptr<runtimeTypes::klass::AnnotationDefinition> findAnnotation(const std::string& name) const
        {
            return findItem(name);
        }

        bool hasAnnotation(const std::string& name) const { return hasItem(name); }
    };
}
