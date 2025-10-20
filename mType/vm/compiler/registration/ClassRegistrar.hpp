#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "InterfaceRegistrar.hpp"
#include "ClassInheritanceValidator.hpp"
#include <memory>
#include <unordered_set>

namespace vm::compiler::registration
{
    /**
     * Registers classes and their metadata for bytecode compilation
     * Handles class definition creation, inheritance linking, and metadata extraction
     */
    class ClassRegistrar
    {
    public:
        ClassRegistrar(
            std::shared_ptr<environment::Environment> environment,
            bytecode::BytecodeProgram& program,
            InterfaceRegistrar* interfaceRegistrar = nullptr
        );

        ~ClassRegistrar() = default;

        // Main registration methods
        void registerClassesForBytecode(ast::ASTNode* node);
        void linkParentClasses(ast::ASTNode* node);

        // Convenience aliases for cleaner API
        void registerClasses(ast::ASTNode* node) { registerClassesForBytecode(node); }
        void linkParents(ast::ASTNode* node) { linkParentClasses(node); }

        // Metadata extraction
        bytecode::BytecodeProgram::ClassMetadata extractClassMetadata(ast::ClassNode* classNode) const;

    private:
        std::shared_ptr<environment::Environment> environment;
        bytecode::BytecodeProgram& program;
        InterfaceRegistrar* interfaceRegistrar;
        std::unique_ptr<ClassInheritanceValidator> inheritanceValidator;

        // Helper methods
        void registerSingleClass(ast::ClassNode* classNode);
        void linkSingleClass(ast::ClassNode* classNode);
        void checkCircularInheritance(
            const std::string& className,
            const std::string& parentClassName,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentDef
        ) const;

        // Validation methods
        void validateParentClassExists(
            const std::string& parentClassName,
            const ast::SourceLocation& location
        ) const;
        void validateInheritanceDepth(
            const std::string& className,
            const ast::SourceLocation& location
        ) const;
        void validateMethodOverrides(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
            const ast::SourceLocation& location
        ) const;
    };
}
