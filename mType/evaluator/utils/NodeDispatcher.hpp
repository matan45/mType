#pragma once
#include "../../value/ValueType.hpp"
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <memory>

// Forward declaration
namespace ast {
    class ASTNode;
}

namespace evaluator::utils
{
    using namespace value;
    using namespace ast;

    /**
     * @brief Generic node dispatcher using type-based lookup
     * Eliminates duplicate dynamic_cast chains across evaluator classes
     * Provides O(1) dispatch instead of O(n) cascading if statements
     *
     * Design follows Command Pattern with type-indexed dispatch table
     */
    template<typename EvaluatorType>
    class NodeDispatcher
    {
    public:
        using DispatchFunc = std::function<Value(EvaluatorType*, ASTNode*)>;

        /**
         * @brief Register a handler for a specific node type
         * @tparam NodeType The AST node type to handle
         * @param handler Function that handles this node type
         */
        template<typename NodeType>
        void registerHandler(DispatchFunc handler)
        {
            dispatchTable[typeid(NodeType)] = handler;
        }

        /**
         * @brief Register a handler using a member function pointer
         * @tparam NodeType The AST node type to handle
         * @param method Member function pointer to evaluation method
         */
        template<typename NodeType>
        void registerMethod(Value (EvaluatorType::*method)(NodeType*))
        {
            dispatchTable[typeid(NodeType)] = [method](EvaluatorType* evaluator, ASTNode* node) {
                return (evaluator->*method)(static_cast<NodeType*>(node));
            };
        }

        /**
         * @brief Dispatch node to appropriate handler
         * @param evaluator The evaluator instance
         * @param node The node to evaluate
         * @return Evaluation result
         */
        Value dispatch(EvaluatorType* evaluator, ASTNode* node)
        {
            if (!node)
            {
                return std::monostate{};
            }

            auto it = dispatchTable.find(typeid(*node));
            if (it != dispatchTable.end())
            {
                return it->second(evaluator, node);
            }

            return std::monostate{};
        }

        /**
         * @brief Check if a handler is registered for this node type
         * @param node The node to check
         * @return true if handler exists, false otherwise
         */
        bool hasHandler(ASTNode* node) const
        {
            return node && dispatchTable.find(typeid(*node)) != dispatchTable.end();
        }

        /**
         * @brief Get number of registered handlers
         * @return Handler count
         */
        size_t handlerCount() const
        {
            return dispatchTable.size();
        }

    private:
        std::unordered_map<std::type_index, DispatchFunc> dispatchTable;
    };
}
