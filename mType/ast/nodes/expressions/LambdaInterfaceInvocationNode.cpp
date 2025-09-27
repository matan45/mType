#include "LambdaInterfaceInvocationNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include <sstream>

namespace ast::nodes::expressions
{
    LambdaInterfaceInvocationNode::LambdaInterfaceInvocationNode(
        std::shared_ptr<LambdaNode> lambda,
        const std::vector<std::shared_ptr<ast::ASTNode>>& args,
        const std::string& interface,
        const std::string& method,
        const std::vector<value::ValueType>& paramTypes,
        value::ValueType returnType
    ) : lambdaNode(lambda), arguments(args), interfaceName(interface), methodName(method),
        interfaceParameterTypes(paramTypes), interfaceReturnType(returnType)
    {
        // Validate that lambda and interface signatures are compatible
        if (lambda && lambda->getParameters().size() != paramTypes.size()) {
            // Parameter count mismatch - this should be caught during semantic analysis
            // For now, we'll allow it and let runtime validation handle it
        }
    }

    value::Value LambdaInterfaceInvocationNode::accept(ast::ASTVisitor<value::Value>& visitor)
    {
        // For now, return a placeholder Value
        // The actual implementation will be handled by the interpreter
        // when it recognizes this special node type
        return value::Value(); // Default constructed value
    }

    std::string LambdaInterfaceInvocationNode::toString() const
    {
        std::stringstream ss;
        ss << "LambdaInterfaceInvocation(";
        ss << "interface: " << interfaceName;
        ss << ", method: " << methodName;
        ss << ", args: [";

        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "ASTNode@" << static_cast<void*>(arguments[i].get());
        }

        ss << "], lambda: ";
        auto lambda = lambdaNode.lock();
        if (lambda) {
            ss << "LambdaNode@" << static_cast<void*>(lambda.get());
        } else {
            ss << "expired";
        }
        ss << ")";

        return ss.str();
    }

    bool LambdaInterfaceInvocationNode::validateParameterTypes(const std::vector<value::Value>& actualArgs) const
    {
        auto lambda = lambdaNode.lock();
        if (!lambda) {
            return false; // Lambda has been destroyed
        }

        const auto& lambdaParams = lambda->getParameters();

        if (actualArgs.size() != lambdaParams.size() ||
            actualArgs.size() != interfaceParameterTypes.size()) {
            return false; // Parameter count mismatch
        }

        // Check type compatibility for each parameter
        for (size_t i = 0; i < actualArgs.size(); ++i) {
            value::ValueType actualType = value::getValueType(actualArgs[i]);
            value::ValueType interfaceType = interfaceParameterTypes[i];

            // Use the enhanced type conversion utils for compatibility checking
            if (!types::TypeConversionUtils::areTypesCompatible(actualType, interfaceType)) {
                return false;
            }
        }

        return true;
    }

    bool LambdaInterfaceInvocationNode::isParameterTypeCompatible(
        value::ValueType interfaceType,
        value::ValueType lambdaType) const
    {
        return types::TypeConversionUtils::areTypesCompatible(lambdaType, interfaceType);
    }
}