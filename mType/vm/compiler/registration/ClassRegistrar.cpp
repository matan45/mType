#include "ClassRegistrar.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../types/TypeSubstitutionService.hpp"

namespace vm::compiler::registration
{
    ClassRegistrar::ClassRegistrar(
        std::shared_ptr<environment::Environment> environment,
        bytecode::BytecodeProgram& program,
        InterfaceRegistrar* interfaceRegistrar
    )
        : environment(environment)
        , program(program)
        , interfaceRegistrar(interfaceRegistrar)
        , inheritanceValidator(std::make_unique<ClassInheritanceValidator>(environment->getClassRegistry()))
        , typeSubstitutionService(std::make_shared<::types::TypeSubstitutionService>())
    {
    }

    void ClassRegistrar::registerClassesForBytecode(ast::ASTNode* node)
    {
        if (!node) return;
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            registerSingleClass(classNode);
            return;
        }

        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                registerClassesForBytecode(importNode->getImportedAST());
            }
        }
    }

    void ClassRegistrar::validateAllClassesHaveBytecode(ast::ASTNode* node)
    {
        if (!node || !compileTimeValidator) return;

        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            compileTimeValidator->validateAllMethodsHaveBytecode(
                classNode->getClassName(),
                classNode->getLocation()
            );
            return;
        }

        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                validateAllClassesHaveBytecode(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                validateAllClassesHaveBytecode(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                validateAllClassesHaveBytecode(importNode->getImportedAST());
            }
        }
    }

    void ClassRegistrar::linkParentClasses(ast::ASTNode* node)
    {
        if (!node) return;

        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            linkSingleClass(classNode);
            return;
        }

        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                linkParentClasses(importNode->getImportedAST());
            }
        }
    }
}
