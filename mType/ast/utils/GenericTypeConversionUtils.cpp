#include "GenericTypeConversionUtils.hpp"
#include <stdexcept>

namespace ast::utils
{
    std::shared_ptr<GenericType> GenericTypeConversionUtils::convertValueTypeToGenericType(value::ValueType type)
    {
        return std::make_shared<GenericType>(type);
    }

    value::ValueType GenericTypeConversionUtils::convertGenericTypeToValueType(const std::shared_ptr<GenericType>& genericType)
    {
        if (!genericType)
        {
            throw std::invalid_argument("GenericTypeConversionUtils::convertGenericTypeToValueType - null genericType pointer");
        }

        if (genericType->isGenericParameter())
        {
            // Generic type parameters (T, E, K, V) represent any reference type at runtime
            return value::ValueType::OBJECT;
        }

        return genericType->getConcreteType();
    }

    std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>
        GenericTypeConversionUtils::convertParametersToGenericType(const std::vector<std::pair<std::string, value::ValueType>>& params)
    {
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> result;
        result.reserve(params.size());

        for (const auto& param : params)
        {
            result.emplace_back(param.first, std::make_shared<GenericType>(param.second));
        }

        return result;
    }

    std::vector<std::pair<std::string, value::ValueType>>
        GenericTypeConversionUtils::convertParametersToValueType(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params)
    {
        std::vector<std::pair<std::string, value::ValueType>> result;
        result.reserve(params.size());

        for (const auto& [paramName, genericType] : params)
        {
            if (!genericType)
            {
                throw std::invalid_argument("GenericTypeConversionUtils::convertParametersToValueType - null GenericType in parameter '" + paramName + "'");
            }

            value::ValueType type = genericType->isGenericParameter()
                ? value::ValueType::OBJECT
                : genericType->getConcreteType();
            result.emplace_back(paramName, type);
        }

        return result;
    }

    std::shared_ptr<GenericType> GenericTypeConversionUtils::cloneGenericType(const std::shared_ptr<GenericType>& genericType)
    {
        return genericType ? std::make_shared<GenericType>(*genericType) : nullptr;
    }

    std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>
        GenericTypeConversionUtils::cloneGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params)
    {
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> result;
        result.reserve(params.size());

        for (const auto& [paramName, genericType] : params)
        {
            if (!genericType)
            {
                throw std::invalid_argument("GenericTypeConversionUtils::cloneGenericParameters - null GenericType in parameter '" + paramName + "'");
            }

            auto clonedGenericType = std::make_shared<GenericType>(*genericType);
            result.emplace_back(paramName, clonedGenericType);
        }

        return result;
    }
}
