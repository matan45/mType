#include "LambdaConversionUtils.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../constants/LambdaConstants.hpp"
#include "../../errors/TypeException.hpp"
#include <variant>

namespace evaluator::utils
{
    Value LambdaConversionUtils::convertLambdaToInterface(const Value& lambdaValue,
                                                         const std::string& interfaceName,
                                                         std::shared_ptr<EvaluationContext> context,
                                                         const SourceLocation& location)
    {
        // Extract the lambda value
        auto lambdaPtr = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
        auto* lambdaNode = lambdaPtr->getLambda();

        // Extract base interface name (e.g., "Function" from "Function<Int,String>")
        std::string baseInterfaceName = interfaceName;
        size_t anglePos = interfaceName.find('<');
        if (anglePos != std::string::npos) {
            baseInterfaceName = interfaceName.substr(0, anglePos);
        }

        // Get the interface definition from the environment using base name
        auto env = context->getEnvironment();
        auto interfaceDef = env->findInterface(baseInterfaceName);

        if (!interfaceDef)
        {
            return lambdaValue;
        }

        // Check if the interface is functional (has exactly one method)
        if (!interfaceDef->isFunctionalInterface())
        {
            auto methodSignatures = interfaceDef->getMethodSignatures();
            throw TypeException(
                "Cannot assign lambda to non-functional interface '" + interfaceName + "'. " +
                "Lambdas can only be assigned to interfaces with exactly one method. " +
                "Interface '" + interfaceName + "' has " + std::to_string(methodSignatures.size()) + " methods. " +
                "Consider using a functional interface (single method) or implement the interface explicitly.",
                location
            );
        }

        // Create the lambda implementation class
        // Pass the full interface name (with generics) so it's stored correctly
        auto implClass = interfaceDef->createLambdaImplementation(lambdaNode, interfaceName);
        if (!implClass)
        {
            return lambdaValue;
        }

        // Create an instance of the implementation class
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(implClass);

        // Store the lambda in a special field that the implementation can access
        instance->setField(constants::lambda::LAMBDA_FIELD_NAME, lambdaValue);

        return instance;
    }
}
