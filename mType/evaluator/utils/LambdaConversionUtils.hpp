#pragma once
#include "../base/EvaluationContext.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>

namespace evaluator::utils
{
    using namespace base;
    using namespace value;
    using namespace errors;

    /**
     * @brief Utility class for lambda to interface conversion
     *
     * Provides shared functionality for converting lambda values to interface implementations.
     * This eliminates code duplication between DeclarationHandler and ImportAndFunctionHandler.
     *
     * Following SOLID principles:
     * - Single Responsibility: Only handles lambda-to-interface conversion
     * - Dependency Inversion: Depends on EvaluationContext abstraction
     */
    class LambdaConversionUtils
    {
    public:
        /**
         * @brief Converts a lambda value to an interface implementation instance
         *
         * @param lambdaValue The lambda value to convert
         * @param interfaceName The name of the target interface
         * @param context The evaluation context containing environment
         * @param location Source location for error reporting
         * @return The converted value (interface instance) or original lambda if conversion not applicable
         * @throws TypeException if interface is not functional
         */
        static Value convertLambdaToInterface(const Value& lambdaValue,
                                            const std::string& interfaceName,
                                            std::shared_ptr<EvaluationContext> context,
                                            const SourceLocation& location = SourceLocation{});

        // Prevent instantiation
        LambdaConversionUtils() = delete;
        ~LambdaConversionUtils() = delete;
        LambdaConversionUtils(const LambdaConversionUtils&) = delete;
        LambdaConversionUtils& operator=(const LambdaConversionUtils&) = delete;
    };
}
