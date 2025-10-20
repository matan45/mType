#pragma once

#include "../base/EvaluationContext.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>
#include <string>

// Forward declarations
namespace runtimeTypes
{
    namespace klass
    {
        class ObjectInstance;
    }
}

namespace evaluator
{
    namespace expressions
    {
        using namespace base;
        using namespace value;
        using namespace ast::nodes::expressions;

        /**
         * @brief Handles type casting and instanceof checking
         *
         * Responsibilities:
         * - Type casting (primitive to primitive, object to object)
         * - instanceof expression evaluation
         * - Class inheritance and interface checking
         * - Generic type handling for casts
         *
         * Design Principles:
         * - Single Responsibility: Only handles type conversions and checks
         * - Dependency Inversion: Depends on IExpressionEvaluator interface
         * - Handles both primitive and object type operations
         */
        class CastAndTypeCheckHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            interfaces::IExpressionEvaluator* exprEvaluator;

        public:
            explicit CastAndTypeCheckHandler(std::shared_ptr<EvaluationContext> ctx);

            void setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator);

            Value evaluateCast(CastExpression* node);
            Value evaluateInstanceOf(InstanceOfExpression* node);

        private:
            // Helper methods for cast operations
            Value castPrimitive(const Value& value, ValueType targetType, const std::string& targetTypeName, const errors::SourceLocation& location);
            Value castObject(const Value& value, const std::string& targetClassName, const errors::SourceLocation& location);
            bool isInstanceOfClass(std::shared_ptr<runtimeTypes::klass::ObjectInstance> objInstance, const std::string& targetClassName);
        };
    }
}
