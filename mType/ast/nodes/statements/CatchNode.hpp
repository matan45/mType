#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <memory>
#include <string>

namespace ast::nodes::statements
{
    /**
     * @brief AST node representing a catch block
     *
     * Syntax: catch (ExceptionType variableName) { ... }
     * Catches exceptions of a specific type and binds them to a variable
     */
    class CatchNode : public ASTNode
    {
    private:
        std::string exceptionType;      // Type of exception to catch (e.g., "NullPointerException")
        std::string variableName;       // Variable name to bind the exception to
        std::unique_ptr<ASTNode> body;  // Catch block body

    public:
        explicit CatchNode(const std::string& exceptionType, const std::string& variableName,
                          std::unique_ptr<ASTNode> body, const SourceLocation& loc = SourceLocation());

        const std::string& getExceptionType() const;
        const std::string& getVariableName() const;
        ASTNode* getBody() const;

        void setExceptionType(const std::string& type);
        void setVariableName(const std::string& name);
        void setBody(std::unique_ptr<ASTNode> catchBody);

        value::Value accept(ASTVisitor<value::Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
