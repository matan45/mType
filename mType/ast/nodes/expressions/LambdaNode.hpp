#pragma once
#include "../../ASTNode.hpp"
#include "../../GenericType.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions {

    struct Parameter {
        std::string name;
        std::shared_ptr<ast::GenericType> type; // Optional, for type inference
            
        Parameter(const std::string& n, 
                 std::shared_ptr<ast::GenericType> t = nullptr)
            : name(n), type(t) {}
    };
        
    enum class BodyType {
        EXPRESSION,  // Single expression: () -> expr
        BLOCK       // Block with statements: () -> { ... }
    };
    
    class LambdaNode : public ASTNode {
    private:
        std::vector<Parameter> parameters;
        std::unique_ptr<ASTNode> body;  // Either expression or block
        BodyType bodyType;
        std::string targetInterface;    // Interface this lambda implements
        std::string targetMethod;        // Method name being implemented
        bool isAsync;  // NEW: Flag to indicate async lambda

    public:
       explicit LambdaNode(const std::vector<Parameter>& params,
                   std::unique_ptr<ASTNode> lambdaBody,
                   const SourceLocation& loc = SourceLocation(),
                   BodyType type = BodyType::EXPRESSION,
                   bool async = false);
        
        [[nodiscard]] const std::vector<Parameter>& getParameters() const;
        [[nodiscard]] ASTNode* getBody() const;
        [[nodiscard]] BodyType getBodyType() const;

        void setTargetInterface(const std::string& interface);
        void setTargetMethod(const std::string& method);

        [[nodiscard]] bool isExpressionLambda() const;
        [[nodiscard]] bool isBlockLambda() const;

        // NEW: Async-related methods
        [[nodiscard]] bool getIsAsync() const { return isAsync; }
        void setIsAsync(bool async) { isAsync = async; }

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}