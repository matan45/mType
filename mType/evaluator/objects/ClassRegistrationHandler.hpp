#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../../value/ValueType.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

// Forward declarations
namespace ast
{
    namespace nodes
    {
        namespace classes
        {
            class ClassNode;
            class InterfaceNode;
        }
    }
}

namespace evaluator
{
    class ExpressionEvaluator;
}

namespace evaluator
{
    namespace objects
    {
        using namespace base;
        using namespace value;
        using namespace runtimeTypes::klass;
        using namespace ast::nodes::classes;

        /**
         * @brief Handles class and interface registration
         *
         * Responsibilities:
         * - Class definition creation and registration
         * - Interface definition creation and registration
         * - Interface implementation validation
         * - Generic type parameter handling
         *
         * Design Principles:
         * - Single Responsibility: Only class/interface registration
         * - Validates interface implementations
         * - Handles generic types and type substitutions
         */
        class ClassRegistrationHandler
        {
        private:
            std::shared_ptr<EvaluationContext> context;
            ExpressionEvaluator* exprEvaluator;

        public:
            explicit ClassRegistrationHandler(std::shared_ptr<EvaluationContext> ctx);
            
            void setExpressionEvaluator(ExpressionEvaluator* evaluator);
           
            
            Value evaluateClass(ClassNode* node);
            
            Value evaluateInterface(InterfaceNode* node);
            
            void registerClass(std::shared_ptr<ClassDefinition> classDef);

        private:

            void validateInterfaceImplementations(std::shared_ptr<ClassDefinition> classDef);
            std::pair<std::string, std::vector<std::string>>
            parseGenericInterfaceName(const std::string& interfaceName);
            std::string resolveGenericType(const std::string& typeName,
                                           const std::unordered_map<std::string, std::string>& substitutions);

            /**
             * @brief Creates a ParameterType with proper interface/class information
             * @param baseType The basic value type
             * @param genericType Optional generic type information
             * @param env Environment for interface/class lookups
             * @return Properly initialized ParameterType with single construction
             */
            ParameterType createParameterType(ValueType baseType,
                                            std::shared_ptr<ast::GenericType> genericType,
                                            environment::Environment* env) const;
        };
    } 
} 
