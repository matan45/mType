#include "LambdaInterfaceInvocationNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../value/ValueTypeUtils.hpp"
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
        // LambdaInterfaceInvocationNode is primarily intended to be handled through
        // the MethodDefinition::hasLambdaNode() path in ObjectEvaluator.
        // However, if this method is called directly, we should handle it gracefully.

        // Check if lambda node is still valid
        if (!isLambdaValid()) {
            throw std::runtime_error("Lambda node expired - cannot invoke interface method '" +
                                   methodName + "' on interface '" + interfaceName + "'");
        }

        // Get the lambda node
        auto lambda = lambdaNode.lock();
        if (!lambda) {
            throw std::runtime_error("Lambda node is null - cannot create lambda for interface method '" +
                                   methodName + "'");
        }

        // We need to delegate to a proper evaluator that can handle lambda evaluation
        // The challenge is that we don't have direct access to the evaluation context here.
        // As a fallback, we'll invoke the lambda's accept method directly.
        try {
            return lambda->accept(visitor);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to evaluate lambda for interface method '" +
                                   methodName + "': " + std::string(e.what()));
        }
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
        return validateParameterTypes(actualArgs, nullptr);
    }

    bool LambdaInterfaceInvocationNode::validateParameterTypes(const std::vector<value::Value>& actualArgs,
                                                              std::string* errorMessage) const
    {
        if (!validateLambdaNodeForParameters(errorMessage)) {
            return false;
        }

        auto lambda = lambdaNode.lock();
        const auto& lambdaParams = lambda->getParameters();

        if (!validateParameterCount(actualArgs, lambdaParams, errorMessage)) {
            return false;
        }

        if (!validateParameterTypeCompatibility(actualArgs, lambdaParams, errorMessage)) {
            return false;
        }

        return true;
    }

    bool LambdaInterfaceInvocationNode::validateLambdaNodeForParameters(std::string* errorMessage) const
    {
        auto lambda = lambdaNode.lock();
        if (!lambda) {
            if (errorMessage) {
                *errorMessage = "Lambda node has been destroyed for interface method '" + methodName +
                               "' in interface '" + interfaceName + "'";
            }
            return false;
        }
        return true;
    }

    bool LambdaInterfaceInvocationNode::validateParameterCount(const std::vector<value::Value>& actualArgs,
                                                               const std::vector<Parameter>& lambdaParams,
                                                               std::string* errorMessage) const
    {
        if (actualArgs.size() != lambdaParams.size()) {
            if (errorMessage) {
                *errorMessage = "Parameter count mismatch for interface method '" + methodName +
                               "': interface expects " + std::to_string(interfaceParameterTypes.size()) +
                               " parameters, lambda has " + std::to_string(lambdaParams.size()) +
                               " parameters, but got " + std::to_string(actualArgs.size()) + " arguments";
            }
            return false;
        }

        if (actualArgs.size() != interfaceParameterTypes.size()) {
            if (errorMessage) {
                *errorMessage = "Interface parameter count mismatch for method '" + methodName +
                               "': expected " + std::to_string(interfaceParameterTypes.size()) +
                               " arguments, got " + std::to_string(actualArgs.size());
            }
            return false;
        }

        return true;
    }

    bool LambdaInterfaceInvocationNode::validateParameterTypeCompatibility(const std::vector<value::Value>& actualArgs,
                                                                          const std::vector<Parameter>& lambdaParams,
                                                                          std::string* errorMessage) const
    {
        for (size_t i = 0; i < actualArgs.size(); ++i) {
            value::ValueType actualType = value::ValueTypeUtils::getValueType(actualArgs[i]);
            value::ValueType interfaceType = interfaceParameterTypes[i];

            if (!types::TypeConversionUtils::areTypesCompatible(actualType, interfaceType)) {
                if (errorMessage) {
                    *errorMessage = "Parameter type mismatch at position " + std::to_string(i) +
                                   " for interface method '" + methodName + "': " +
                                   "expected " + getValueTypeName(interfaceType) +
                                   ", got " + getValueTypeName(actualType) +
                                   " (parameter name: '" + lambdaParams[i].name + "')";
                }
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

    void LambdaInterfaceInvocationNode::setGenericTypeBinding(const std::string& typeParam, value::ValueType actualType)
    {
        genericTypeBindings[typeParam] = actualType;
    }

    value::ValueType LambdaInterfaceInvocationNode::resolveGenericType(const std::string& typeParam) const
    {
        auto it = genericTypeBindings.find(typeParam);
        if (it != genericTypeBindings.end()) {
            return it->second;
        }

        // If no binding found, default to object type
        // In a more sophisticated implementation, this might throw an error
        return value::ValueType::OBJECT;
    }

    bool LambdaInterfaceInvocationNode::validateReturnType(const value::Value& returnValue) const
    {
        return validateReturnType(returnValue, nullptr);
    }

    bool LambdaInterfaceInvocationNode::validateReturnType(const value::Value& returnValue, std::string* errorMessage) const
    {
        value::ValueType actualReturnType = value::ValueTypeUtils::getValueType(returnValue);

        if (!isReturnTypeCompatible(actualReturnType)) {
            if (errorMessage) {
                *errorMessage = "Return type mismatch for interface method '" + methodName +
                               "': expected " + getValueTypeName(interfaceReturnType) +
                               ", got " + getValueTypeName(actualReturnType);
            }
            return false;
        }

        return true;
    }

    bool LambdaInterfaceInvocationNode::isReturnTypeCompatible(value::ValueType actualReturnType) const
    {
        // Use the same compatibility logic as parameter types
        return types::TypeConversionUtils::areTypesCompatible(actualReturnType, interfaceReturnType);
    }

    bool LambdaInterfaceInvocationNode::isLambdaAccessible() const
    {
        ++accessCount; // Track access attempts

        if (markedAsInvalid) {
            return false;
        }

        return !lambdaNode.expired();
    }

    void LambdaInterfaceInvocationNode::markAsInvalid()
    {
        markedAsInvalid = true;
        lambdaNode.reset(); // Clear the weak_ptr
    }

    std::string LambdaInterfaceInvocationNode::getLambdaLifecycleStatus() const
    {
        std::stringstream ss;
        ss << "LambdaInterfaceInvocationNode(interface=" << interfaceName
           << ", method=" << methodName << ", accessCount=" << accessCount;

        if (markedAsInvalid) {
            ss << ", status=MARKED_INVALID";
        } else if (lambdaNode.expired()) {
            ss << ", status=LAMBDA_EXPIRED";
        } else {
            ss << ", status=ACTIVE";
        }

        ss << ")";
        return ss.str();
    }

    void LambdaInterfaceInvocationNode::cleanup()
    {
        // Clear references
        lambdaNode.reset();
        arguments.clear();
        genericTypeBindings.clear();

        // Mark as cleaned up
        markedAsInvalid = true;
    }

    bool LambdaInterfaceInvocationNode::needsCleanup() const
    {
        return !markedAsInvalid && lambdaNode.expired();
    }

    std::string LambdaInterfaceInvocationNode::getValueTypeName(value::ValueType type) const
    {
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::STRING: return "string";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::LAMBDA: return "lambda";
            case value::ValueType::ARRAY: return "array";
            default: return "unknown";
        }
    }

    std::unique_ptr<ASTNode> LambdaInterfaceInvocationNode::clone() const
    {
        // Clone arguments
        std::vector<std::shared_ptr<ASTNode>> clonedArgs;
        clonedArgs.reserve(arguments.size());
        for (const auto& arg : arguments) {
            if (arg) {
                clonedArgs.push_back(std::shared_ptr<ASTNode>(arg->clone()));
            }
        }

        // Get the shared_ptr to lambda (if valid)
        std::shared_ptr<LambdaNode> lambdaPtr = lambdaNode.lock();

        // Create cloned invocation node
        auto clonedNode = std::make_unique<LambdaInterfaceInvocationNode>(
            lambdaPtr,
            clonedArgs,
            interfaceName,
            methodName,
            interfaceParameterTypes,
            interfaceReturnType
        );

        // Copy generic type bindings
        for (const auto& [typeParam, actualType] : genericTypeBindings) {
            clonedNode->setGenericTypeBinding(typeParam, actualType);
        }

        return clonedNode;
    }
}