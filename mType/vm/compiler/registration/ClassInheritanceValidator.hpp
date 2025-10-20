#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../environment/registry/ClassRegistry.hpp"
#include <string>
#include <memory>

namespace vm::compiler::registration
{
    /**
     * Validates class inheritance depth
     * Extracted from ClassRegistrar to improve Single Responsibility Principle compliance
     * Ensures inheritance hierarchies don't exceed reasonable depth limits
     */
    class ClassInheritanceValidator
    {
    public:
        explicit ClassInheritanceValidator(std::shared_ptr<environment::registry::ClassRegistry> registry);
        ~ClassInheritanceValidator() = default;

        /**
         * Validate inheritance depth doesn't exceed limit
         * @param className The class name to check
         * @param location Source location for error reporting
         */
        void validateInheritanceDepth(const std::string& className,
                                     const ast::SourceLocation& location) const;

        // Maximum inheritance depth constant
        static constexpr size_t MAX_INHERITANCE_DEPTH = 100;

    private:
        std::shared_ptr<environment::registry::ClassRegistry> classRegistry;
    };
}
