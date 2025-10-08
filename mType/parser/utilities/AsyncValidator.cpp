#include "AsyncValidator.hpp"
#include "../../errors/ParseException.hpp"
#include "../../value/ValueType.hpp"

namespace parser::utilities
{
    using namespace value;

    void AsyncValidator::validateAsyncReturnType(
        const std::shared_ptr<GenericType>& returnType,
        const SourceLocation& location)
    {
        if (!returnType)
        {
            throw errors::ParseException(
                "Async functions must have a return type (expected Promise<T>)",
                location);
        }

        // Check if return type is Promise<T>
        if (!isPromiseType(returnType))
        {
            throw errors::ParseException(
                "Async functions must return Promise<T> type. Found: " + returnType->toString(),
                location);
        }

        // Validate that Promise has exactly one type argument
        const auto& typeArgs = returnType->getTypeArguments();
        if (typeArgs.size() != 1)
        {
            throw errors::ParseException(
                "Promise type must have exactly one type argument: Promise<T>",
                location);
        }
    }

    bool AsyncValidator::isPromiseType(const std::shared_ptr<GenericType>& type)
    {
        if (!type)
        {
            return false;
        }

        // Check if the base type name is "Promise"
        std::string baseTypeName = type->getBaseTypeName();

        // Check if it's named "Promise"
        return baseTypeName == "Promise";
    }

    std::shared_ptr<GenericType> AsyncValidator::unwrapPromiseType(
        const std::shared_ptr<GenericType>& promiseType)
    {
        if (!isPromiseType(promiseType))
        {
            return nullptr;
        }

        const auto& typeArgs = promiseType->getTypeArguments();
        if (typeArgs.empty())
        {
            return nullptr;
        }

        return typeArgs[0];
    }

    std::shared_ptr<GenericType> AsyncValidator::wrapInPromise(
        const std::shared_ptr<GenericType>& innerType)
    {
        if (!innerType)
        {
            // Default to Promise<void>
            return std::make_shared<GenericType>(
                "Promise",
                std::vector<std::shared_ptr<GenericType>>{
                    std::make_shared<GenericType>(ValueType::VOID)
                });
        }

        // Create Promise<innerType>
        return std::make_shared<GenericType>(
            "Promise",
            std::vector<std::shared_ptr<GenericType>>{innerType});
    }

    void AsyncValidator::validateAwaitContext(
        bool isAsyncContext,
        const SourceLocation& location)
    {
        if (!isAsyncContext)
        {
            throw errors::ParseException(
                "'await' can only be used inside async functions",
                location);
        }
    }
}
