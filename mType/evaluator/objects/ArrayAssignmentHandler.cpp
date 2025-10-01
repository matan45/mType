#include "ArrayAssignmentHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../../errors/TypeException.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../ast/nodes/statements/IndexAssignmentNode.hpp"

namespace evaluator {
namespace objects {

    Value ArrayAssignmentHandler::evaluateIndexAssignment(IndexAssignmentNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for index assignment");
        }

        // Try to extract multi-dimensional assignment (e.g., arr[i][j][k] = value)
        auto multiDimResult = extractMultiDimensionalAssignment(node);
        if (multiDimResult.has_value())
        {
            auto [baseArray, indices] = multiDimResult.value();
            Value newValue = exprEvaluator->evaluate(node->getValue());
            return performDirectMultiDimensionalAssignment(baseArray, indices, newValue, node->getLocation());
        }

        // Check if this is a multi-dimensional assignment (e.g., arr2d[0][0] = value)
        auto objectASTNode = node->getObject();

        // Try to detect if this is an IndexAccessNode
        if (auto indexAccessNode = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(objectASTNode))
        {
            // Get the base array
            Value baseArrayValue = exprEvaluator->evaluate(indexAccessNode->getCollection());

            if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue))
            {
                auto baseArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue);
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue))
                {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    if (firstIndex < 0)
                    {
                        throw TypeException(
                            "Multi-dimensional array first index " + std::to_string(firstIndex) + " is negative",
                            node->getLocation());
                    }
                    if (secondIndex < 0)
                    {
                        throw TypeException(
                            "Multi-dimensional array second index " + std::to_string(secondIndex) + " is negative",
                            node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try
                    {
                        baseArray->set(indices, newValue);
                        return newValue;
                    }
                    catch (const std::out_of_range& e)
                    {
                        throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()),
                                            node->getLocation());
                    }
                }
            }
            else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue))
            {
                auto baseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue);
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue))
                {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    if (firstIndex < 0)
                    {
                        throw TypeException(
                            "Sparse multi-dimensional array first index " + std::to_string(firstIndex) + " is negative",
                            node->getLocation());
                    }
                    if (secondIndex < 0)
                    {
                        throw TypeException(
                            "Sparse multi-dimensional array second index " + std::to_string(secondIndex) + " is negative",
                            node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try
                    {
                        baseArray->set(indices, newValue);
                        return newValue;
                    }
                    catch (const std::out_of_range& e)
                    {
                        throw TypeException("Sparse multi-dimensional array assignment failed: " + std::string(e.what()),
                                            node->getLocation());
                    }
                }
            }
        }

        // Regular 1D array or nested array assignment
        Value collectionValue = exprEvaluator->evaluate(node->getObject());
        Value indexValue = exprEvaluator->evaluate(node->getIndex());
        Value newValue = exprEvaluator->evaluate(node->getValue());

        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(collectionValue))
        {
            auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(collectionValue);

            if (!std::holds_alternative<int>(indexValue))
            {
                throw TypeException("Array index must be an integer", node->getLocation());
            }

            int index = std::get<int>(indexValue);
            if (index < 0)
            {
                throw TypeException("Array index cannot be negative: " + std::to_string(index),
                                    node->getLocation());
            }

            size_t arrayIndex = static_cast<size_t>(index);
            if (arrayIndex >= nativeArray->size())
            {
                throw TypeException(
                    "Array index " + std::to_string(index) + " out of bounds for array of size " +
                    std::to_string(nativeArray->size()),
                    node->getLocation());
            }

            nativeArray->set(arrayIndex, newValue);
            return newValue;
        }

        throw TypeException("Cannot perform index assignment on non-array type", node->getLocation());
    }

    std::optional<std::pair<Value, std::vector<size_t>>> ArrayAssignmentHandler::extractMultiDimensionalAssignment(
        IndexAssignmentNode* node)
    {
        std::vector<ASTNode*> indexNodes;
        ASTNode* currentNode = node->getObject();

        // Traverse the chain of IndexAccessNode
        while (auto indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(currentNode))
        {
            indexNodes.insert(indexNodes.begin(), indexAccess->getIndex());
            currentNode = indexAccess->getCollection();
        }

        indexNodes.push_back(node->getIndex());

        // Need at least 3 indices for enhanced multi-dimensional assignment
        if (indexNodes.size() < 3)
        {
            return std::nullopt;
        }

        Value baseArray = exprEvaluator->evaluate(currentNode);

        size_t expectedRank = 0;
        bool isMultiDimensional = false;

        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);
            expectedRank = flatArray->getRank();
            isMultiDimensional = true;
        }
        else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);
            expectedRank = sparseArray->getRank();
            isMultiDimensional = true;
        }
        else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(baseArray))
        {
            expectedRank = indexNodes.size();
            isMultiDimensional = true;
        }

        if (!isMultiDimensional)
        {
            return std::nullopt;
        }

        if ((std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray) ||
             std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray)) &&
            indexNodes.size() != expectedRank)
        {
            return std::nullopt;
        }

        std::vector<size_t> indices;
        indices.reserve(indexNodes.size());

        for (auto it = indexNodes.begin(); it != indexNodes.end(); ++it)
        {
            Value indexValue = exprEvaluator->evaluate(*it);

            if (!std::holds_alternative<int>(indexValue))
            {
                return std::nullopt;
            }

            int index = std::get<int>(indexValue);
            if (index < 0)
            {
                return std::nullopt;
            }

            indices.push_back(static_cast<size_t>(index));
        }

        return std::make_pair(baseArray, indices);
    }

    Value ArrayAssignmentHandler::performDirectMultiDimensionalAssignment(const Value& baseArray,
                                                                   const std::vector<size_t>& indices,
                                                                   const Value& newValue,
                                                                   const SourceLocation& location)
    {
        // Handle FlatMultiArray
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);

            try
            {
                flatArray->set(indices, newValue);
                return newValue;
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()), location);
            }
        }

        // Handle SparseMultiArray
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);

            try
            {
                sparseArray->set(indices, newValue);
                return newValue;
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Sparse multi-dimensional array assignment failed: " + std::string(e.what()),
                                    location);
            }
        }

        // Handle NativeArray (jagged arrays)
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(baseArray))
        {
            if (indices.empty())
            {
                throw TypeException("No indices provided for jagged array assignment", location);
            }

            try
            {
                // Navigate through nested NativeArrays
                Value currentArray = baseArray;
                for (size_t i = 0; i < indices.size() - 1; ++i)
                {
                    if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(currentArray))
                    {
                        throw TypeException("Expected nested array at dimension " + std::to_string(i) +
                                          " but found different type", location);
                    }

                    auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(currentArray);
                    size_t index = indices[i];

                    if (index >= nativeArray->size())
                    {
                        throw TypeException("Index " + std::to_string(index) + " out of bounds for dimension " +
                                          std::to_string(i) + " (size: " + std::to_string(nativeArray->size()) + ")",
                                          location);
                    }

                    currentArray = nativeArray->get(index);

                    if (std::holds_alternative<std::monostate>(currentArray))
                    {
                        throw TypeException("Null array encountered at dimension " + std::to_string(i) +
                                          ". Initialize sub-array before assignment.", location);
                    }
                }

                // Final assignment
                if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(currentArray))
                {
                    throw TypeException("Expected array for final assignment but found different type", location);
                }

                auto finalArray = std::get<std::shared_ptr<value::NativeArray>>(currentArray);
                size_t finalIndex = indices.back();

                if (finalIndex >= finalArray->size())
                {
                    throw TypeException("Final index " + std::to_string(finalIndex) + " out of bounds " +
                                      "(size: " + std::to_string(finalArray->size()) + ")", location);
                }

                finalArray->set(finalIndex, newValue);
                return newValue;
            }
            catch (const TypeException&)
            {
                throw;
            }
            catch (const std::exception& e)
            {
                throw TypeException("Jagged array assignment failed: " + std::string(e.what()), location);
            }
        }

        throw TypeException("Unsupported array type for direct multi-dimensional assignment", location);
    }

} // namespace objects
} // namespace evaluator
