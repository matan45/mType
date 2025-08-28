#pragma once
#include "../../ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include <string>
#include <vector>
#include <functional>

namespace ast::nodes::statements
{
    using namespace value;

    // Forward declaration for function signature
    using NativeFunction = std::function<Value(const std::vector<Value>&)>;

    class NativeFunctionNode : public ASTNode
    {
    private:
        std::string functionName;
        ValueType returnType;
        std::vector<ValueType> parameterTypes;
        NativeFunction implementation;

    public:
        explicit NativeFunctionNode(const std::string& name, ValueType retType,
                                   std::vector<ValueType> paramTypes,
                                   NativeFunction impl,
                                   const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), functionName(name), returnType(retType), 
              parameterTypes(std::move(paramTypes)), implementation(std::move(impl)) {}

        const std::string& getFunctionName() const { return functionName; }
        ValueType getReturnType() const { return returnType; }
        const std::vector<ValueType>& getParameterTypes() const { return parameterTypes; }
        const NativeFunction& getImplementation() const { return implementation; }

        size_t getParameterCount() const { return parameterTypes.size(); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitNativeFunctionNode(this);
        }
    };
}