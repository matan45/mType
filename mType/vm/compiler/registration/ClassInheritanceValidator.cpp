#include "ClassInheritanceValidator.hpp"
#include "../../../errors/InheritanceException.hpp"
#include <sstream>

namespace vm::compiler::registration
{
    ClassInheritanceValidator::ClassInheritanceValidator(std::shared_ptr<environment::registry::ClassRegistry> registry)
        : classRegistry(registry)
    {
    }

    void ClassInheritanceValidator::validateInheritanceDepth(
        const std::string& className,
        const ast::SourceLocation& location) const
    {
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        auto classDef = classRegistry->findClass(className);
        if (!classDef) {
            return;
        }

        // Count inheritance depth
        size_t depth = 0;
        auto currentClass = classDef->getParentClass();

        while (currentClass) {
            depth++;
            if (depth >= MAX_INHERITANCE_DEPTH) {
                // Build inheritance chain for error message
                std::stringstream chainMsg;
                chainMsg << "Inheritance chain exceeds maximum depth of "
                         << MAX_INHERITANCE_DEPTH << ": " << className;

                // Traverse again to build the chain string
                auto tempClass = classDef->getParentClass();
                while (tempClass && depth-- > 0) {
                    chainMsg << " -> " << tempClass->getName();
                    tempClass = tempClass->getParentClass();
                }

                throw errors::InheritanceException(
                    chainMsg.str(),
                    location
                );
            }
            currentClass = currentClass->getParentClass();
        }
    }
}
