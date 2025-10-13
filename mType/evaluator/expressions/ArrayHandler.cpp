#include "ArrayHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../../errors/TypeException.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../value/ArrayPool.hpp"
#include "../../value/arrays/ArrayFactory.hpp"
#include "../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../evaluator/utils/ObjectHelper.hpp"

using namespace errors;
using namespace evaluator::utils;

namespace evaluator {
namespace expressions {

    Value ArrayHandler::getDefaultValueForType(const ::parser::TypeInfo& elementType)
    {
        switch (elementType.baseType)
        {
        case ValueType::INT:
            return 0;
        case ValueType::FLOAT:
            return 0.0f;
        case ValueType::STRING:
            return std::string("");
        case ValueType::BOOL:
            return false;
        default:
            return std::monostate{}; // null for objects and generics
        }
    }

    ArrayHandler::ArrayHandler(std::shared_ptr<EvaluationContext> ctx)
    : context(ctx), exprEvaluator(nullptr) {}

    void ArrayHandler::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
    }

    Value ArrayHandler::evaluateArrayCreation(ArrayCreationNode* node)
    {
        // Get all size expressions for multidimensional support
        const auto& sizeExpressions = node->getSizeExpressions();

        if (sizeExpressions.empty())
        {
            throw TypeException("Array creation must have at least one dimension", node->getLocation());
        }

        // Evaluate size expressions and handle jagged arrays
        std::vector<size_t> dimensions;
        bool hasJaggedDimensions = false;

        for (const auto& sizeExpr : sizeExpressions)
        {
            if (sizeExpr == nullptr)
            {
                // Null expression indicates jagged dimension (e.g., new int[2][])
                hasJaggedDimensions = true;
                dimensions.push_back(0); // Use 0 to indicate unspecified size
            }
            else
            {
                Value sizeValue = exprEvaluator->evaluate(sizeExpr.get());

                if (!std::holds_alternative<int>(sizeValue))
                {
                    throw TypeException("Array size must be an integer", node->getLocation());
                }

                int size = std::get<int>(sizeValue);
                if (size < 0)
                {
                    throw TypeException("Array size cannot be negative", node->getLocation());
                }
                dimensions.push_back(static_cast<size_t>(size));
            }
        }

        // Determine default value based on element type
        Value defaultValue = getDefaultValueForType(node->getElementTypeInfo());

        // Handle different array creation scenarios
        if (dimensions.size() == 1)
        {
            // Use ArrayFactory for optimized 1D array creation
            auto elementTypeInfo = node->getElementTypeInfo();
            auto classRegistry = context->getEnvironment()->getClassRegistry().get();

            auto nativeArray = mType::value::arrays::ArrayFactory::create1DArray(
                dimensions[0],
                elementTypeInfo.baseType,
                elementTypeInfo.className,
                nullptr,  // ClassDefinition will be resolved by ArrayFactory
                classRegistry
            );

            // Initialize with default values
            for (size_t i = 0; i < dimensions[0]; ++i)
            {
                nativeArray->set(i, defaultValue);
            }
            return nativeArray;
        }
        else if (hasJaggedDimensions)
        {
            // Handle jagged arrays (e.g., new int[2][] or new int[2][][])
            // Find the first specified dimension
            size_t firstDimension = 0;
            bool foundSpecifiedDimension = false;

            for (size_t dim : dimensions)
            {
                if (dim != 0)
                {
                    firstDimension = dim;
                    foundSpecifiedDimension = true;
                    break;
                }
            }

            if (!foundSpecifiedDimension)
            {
                throw TypeException("Jagged arrays must have at least one specified dimension", node->getLocation());
            }

            // Create an array with the first specified dimension using ArrayFactory
            auto elementTypeInfo = node->getElementTypeInfo();
            auto classRegistry = context->getEnvironment()->getClassRegistry().get();

            auto jaggedArray = mType::value::arrays::ArrayFactory::create1DArray(
                firstDimension,
                elementTypeInfo.baseType,
                elementTypeInfo.className,
                nullptr,
                classRegistry
            );

            // Initialize each element to null (will be assigned later)
            for (size_t i = 0; i < firstDimension; ++i)
            {
                jaggedArray->set(i, std::monostate{}); // null until assigned
            }

            return jaggedArray;
        }
        else
        {
            // Multi-dimensional array creation
            auto elementTypeInfo = node->getElementTypeInfo();
            auto classRegistry = context->getEnvironment()->getClassRegistry().get();

            // Try ArrayFactory for multi-dimensional arrays (supports FlatMultiObjectArray for objects)
            Value adaptiveArray = mType::value::arrays::ArrayFactory::createMultiDimensionalArray(
                dimensions,
                elementTypeInfo.baseType,
                elementTypeInfo.className,
                nullptr,  // ClassDefinition resolved by ArrayFactory
                classRegistry
            );

            // If it's not a FlatMultiObjectArray, use the existing ArrayPool adaptive logic
            if (!std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(adaptiveArray))
            {
                // Fallback to ArrayPool for non-object multi-dimensional arrays
                auto& pool = ArrayPool::getInstance();
                adaptiveArray = pool.acquireAdaptive(dimensions, defaultValue);
            }

            // Verify the array is valid (works for both FlatMultiArray and SparseMultiArray)
            if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(adaptiveArray))
            {
                auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(adaptiveArray);
                if (!flatArray)
                {
                    throw TypeException("Failed to create FlatMultiArray", node->getLocation());
                }

                // Verify size for dense arrays
                size_t expectedSize = 1;
                for (size_t dim : dimensions)
                {
                    expectedSize *= dim;
                }
                if (flatArray->totalSize() != expectedSize)
                {
                    throw TypeException("FlatMultiArray size mismatch", node->getLocation());
                }
            }
            else if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(adaptiveArray))
            {
                auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(adaptiveArray);
                if (!sparseArray)
                {
                    throw TypeException("Failed to create SparseMultiArray", node->getLocation());
                }

                // Verify dimensions for sparse arrays
                if (!sparseArray->hasDimensions(dimensions))
                {
                    throw TypeException("SparseMultiArray dimension mismatch", node->getLocation());
                }
            }
            else
            {
                throw TypeException("Unknown array type returned from adaptive pool", node->getLocation());
            }

            return adaptiveArray;
        }
    }

    Value ArrayHandler::evaluateArrayLiteral(ArrayLiteralNode* node)
    {
        const auto& elements = node->getElements();

        if (elements.empty())
        {
            // Create empty array with size 0
            auto emptyArray = std::make_shared<NativeArray>(0);
            return emptyArray;
        }

        // First evaluate all elements to determine array size and validate types
        std::vector<Value> evaluatedElements;
        evaluatedElements.reserve(elements.size());
        ValueType expectedType = ValueType::VOID; // Will be set from first element
        bool isFirstElement = true;

        for (const auto& element : elements)
        {
            Value elementValue = exprEvaluator->evaluate(element.get());
            ValueType currentType = value::getValueType(elementValue); // Use global getValueType

            if (isFirstElement)
            {
                // Set expected type from first element
                expectedType = currentType;
                isFirstElement = false;
            }
            else
            {
                // Validate that all subsequent elements have the same type
                if (currentType != expectedType)
                {
                    // Special case: allow int in float arrays (implicit conversion)
                    if (expectedType == ValueType::FLOAT && currentType == ValueType::INT)
                    {
                        // Convert int to float
                        elementValue = static_cast<float>(std::get<int>(elementValue));
                    }
                    // Special case: null is not allowed in primitive arrays
                    else if (currentType == ValueType::VOID || currentType == ValueType::NULL_TYPE)
                    {
                        throw TypeException("Cannot mix null values with primitive types in array literals", node->getLocation());
                    }
                    else
                    {
                        // Get readable type names for error message
                        std::string expectedTypeName = ValueConverter::valueTypeToString(expectedType);
                        std::string actualTypeName = ValueConverter::valueTypeToString(currentType);
                        throw TypeException("Array literal type mismatch: expected " + expectedTypeName +
                                           " but found " + actualTypeName, node->getLocation());
                    }
                }
                else if (currentType == ValueType::OBJECT && expectedType == ValueType::OBJECT)
                {
                    // For objects, we need to validate the class types are compatible
                    if (!ObjectHelper::areObjectTypesCompatible(evaluatedElements[0], elementValue))
                    {
                        std::string expectedClassName = ObjectHelper::getClassName(evaluatedElements[0]);
                        std::string actualClassName = ObjectHelper::getClassName(elementValue);
                        throw TypeException("Array literal object type mismatch: expected " + expectedClassName +
                                           " but found " + actualClassName, node->getLocation());
                    }
                }
            }

            evaluatedElements.push_back(elementValue);
        }

        // Create NativeArray with the correct size and element type using ArrayFactory
        std::string elementTypeName = "";
        if (expectedType == ValueType::OBJECT && !evaluatedElements.empty())
        {
            // For object arrays, store the class name
            elementTypeName = ObjectHelper::getClassName(evaluatedElements[0]);
        }

        auto classRegistry = context->getEnvironment()->getClassRegistry().get();

        auto array = mType::value::arrays::ArrayFactory::createFromLiteral(
            evaluatedElements,
            expectedType,
            elementTypeName,
            classRegistry
        );

        return array;
    }

    Value ArrayHandler::evaluateIndexAccess(IndexAccessNode* node)
    {
        // Check if this is a multi-dimensional access pattern (e.g., arr[i][j])
        std::vector<size_t> indices;
        auto baseArray = extractMultiDimensionalAccess(node, indices);

        if (baseArray.has_value())
        {
            // Handle direct multi-dimensional access
            return evaluateDirectMultiDimensionalAccess(baseArray.value(), indices, node->getLocation());
        }

        // Evaluate the array expression
        Value arrayValue = exprEvaluator->evaluate(node->getCollection());

        // Evaluate the index expression
        Value indexValue = exprEvaluator->evaluate(node->getIndex());

        // Check if index is an integer
        if (!std::holds_alternative<int>(indexValue))
        {
            throw TypeException("Array index must be an integer", node->getLocation());
        }

        int index = std::get<int>(indexValue);

        // Check if array is a NativeArray
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(arrayValue))
        {
            auto nativeArray = std::get<std::shared_ptr<NativeArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException("Array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= nativeArray->size())
            {
                throw TypeException("Array index " + std::to_string(index) + " exceeds array size of " +
                                    std::to_string(nativeArray->size()) + " elements (valid range: 0 to " +
                                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }

            return nativeArray->get(static_cast<size_t>(index));
        }

        // Check if array is a FlatMultiArray (for multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(arrayValue))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Multi-dimensional array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= flatArray->size())
            {
                throw TypeException(
                    "Multi-dimensional array index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(flatArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }

            // For multi-dimensional arrays, return a sub-array view
            if (flatArray->getRank() > 1)
            {
                auto subArray = flatArray->getSubArray(static_cast<size_t>(index));
                if (subArray)
                {
                    return subArray;
                }
                else
                {
                    throw TypeException("Cannot access sub-array", node->getLocation());
                }
            }
            else
            {
                // Single dimension, return the value directly
                try
                {
                    return flatArray->get(static_cast<size_t>(index));
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Array access failed: " + std::string(e.what()), node->getLocation());
                }
            }
        }

        // Check if array is a SparseMultiArray (for adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(arrayValue))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(arrayValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Sparse array index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= sparseArray->size())
            {
                throw TypeException(
                    "Sparse array index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(sparseArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }

            // For sparse multi-dimensional arrays, return a sub-array view
            if (sparseArray->getRank() > 1)
            {
                auto subArray = sparseArray->getSubArray(static_cast<size_t>(index));
                if (subArray)
                {
                    return subArray;
                }
                else
                {
                    throw TypeException("Cannot access sub-array in sparse array", node->getLocation());
                }
            }
            else
            {
                // Single dimension sparse array
                std::vector<size_t> indices = {static_cast<size_t>(index)};
                try
                {
                    return sparseArray->get(indices);
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Sparse array access failed: " + std::string(e.what()), node->getLocation());
                }
            }
        }

        throw TypeException("Cannot index non-array value", node->getLocation());
    }

    std::optional<Value> ArrayHandler::extractMultiDimensionalAccess(
        IndexAccessNode* node, std::vector<size_t>& indices)
    {
        // Traverse up the IndexAccessNode chain to collect all indices
        std::vector<IndexAccessNode*> accessChain;
        IndexAccessNode* current = node;

        while (current != nullptr)
        {
            accessChain.push_back(current);

            // Check if the collection is also an IndexAccessNode
            IndexAccessNode* nextAccess = dynamic_cast<IndexAccessNode*>(current->getCollection());
            if (nextAccess != nullptr)
            {
                current = nextAccess;
            }
            else
            {
                break;
            }
        }

        // If we only have one level, this isn't multi-dimensional
        if (accessChain.size() <= 1)
        {
            return std::nullopt;
        }

        // Evaluate the base array (the deepest collection)
        Value baseArray = exprEvaluator->evaluate(accessChain.back()->getCollection());

        // Check if it's a multi-dimensional array type we can optimize
        bool isMultiDimensional = false;
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(baseArray);
            isMultiDimensional = flatArray->getRank() > 1;
        }
        else if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(baseArray);
            isMultiDimensional = sparseArray->getRank() > 1;
        }

        if (!isMultiDimensional)
        {
            return std::nullopt;
        }

        // Evaluate indices in reverse order (from base to leaf)
        indices.clear();
        indices.reserve(accessChain.size());

        for (auto it = accessChain.rbegin(); it != accessChain.rend(); ++it)
        {
            Value indexValue = exprEvaluator->evaluate((*it)->getIndex());

            if (!std::holds_alternative<int>(indexValue))
            {
                return std::nullopt; // Fall back to regular handling
            }

            int index = std::get<int>(indexValue);
            if (index < 0)
            {
                return std::nullopt; // Let regular handling provide proper error
            }

            indices.push_back(static_cast<size_t>(index));
        }

        return baseArray;
    }

    Value ArrayHandler::evaluateDirectMultiDimensionalAccess(const Value& baseArray,
                                                                const std::vector<size_t>& indices,
                                                                const SourceLocation& location)
    {
        // Handle FlatMultiArray direct access
        if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(baseArray);

            try
            {
                return flatArray->get(indices);
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Multi-dimensional array access failed: " + std::string(e.what()), location);
            }
        }

        // Handle SparseMultiArray direct access
        if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(baseArray);

            try
            {
                return sparseArray->get(indices);
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Sparse multi-dimensional array access failed: " + std::string(e.what()), location);
            }
        }

        throw TypeException("Unsupported array type for direct multi-dimensional access", location);
    }

} 
} 
