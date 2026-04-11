#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../environment/Environment.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../types/UnifiedType.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include "InterfaceRegistrar.hpp"
#include "ClassInheritanceValidator.hpp"
#include <memory>
#include <unordered_set>

namespace vm::compiler::validation
{
    class CompileTimeValidator;  // Forward declaration
}

namespace vm::compiler
{
    class BytecodeCompiler;  // Forward declaration — back-pointer for the warning sink
}

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

        // Validator setup (called after construction)
        void setCompileTimeValidator(validation::CompileTimeValidator* validator) { compileTimeValidator = validator; }

        // MYT-35 follow-up — back-pointer to the owning BytecodeCompiler so
        // analyzer-style checks (e.g., MYT-50 missing-@Override) can push
        // non-fatal Diagnostics into the compiler's warning sink.
        void setBytecodeCompiler(BytecodeCompiler* compiler) { compiler_ = compiler; }

        // Main registration methods
        void registerClassesForBytecode(ast::ASTNode* node);
        void linkParentClasses(ast::ASTNode* node);
        void validateAllClassesHaveBytecode(ast::ASTNode* node);

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
        validation::CompileTimeValidator* compileTimeValidator = nullptr;
        BytecodeCompiler* compiler_ = nullptr;  // optional warning sink (MYT-35 follow-up)

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
            ast::ClassNode* classNode
        ) const;

        // Generic type substitution
        void parseAndStoreTypeSubstitutions(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
            const std::string& parentClassNameWithGenerics,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
        ) const;
        std::vector<std::string> parseGenericTypeArguments(const std::string& classNameWithGenerics) const;

        // NEW: UnifiedType-based substitution methods
        void parseAndStoreUnifiedTypeSubstitutions(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
            const std::string& parentClassNameWithGenerics,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
        ) const;

        // Type substitution service (shared across compiler)
        std::shared_ptr<::types::TypeSubstitutionService> typeSubstitutionService;
    };
}
