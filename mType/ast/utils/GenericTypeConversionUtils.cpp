#include "GenericTypeConversionUtils.hpp"

namespace ast::utils
{
    std::shared_ptr<GenericType> GenericTypeConversionUtils::convertValueTypeToGenericType(value::ValueType type)
    {
        return std::make_shared<GenericType>(type);
    }

    value::ValueType GenericTypeConversionUtils::convertGenericTypeToValueType(const std::shared_ptr<GenericType>& genericType)
    {
        if (genericType && !genericType->isGenericParameter())
        {
            return genericType->getConcreteType();
        }
        return value::ValueType::OBJECT; // Default for generic parameters
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

        for (const auto& param : params)
        {
            value::ValueType type = param.second->isGenericParameter()
                ? value::ValueType::OBJECT
                : param.second->getConcreteType();
            result.emplace_back(param.first, type);
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

        for (const auto& param : params)
        {
            auto clonedGenericType = std::make_shared<GenericType>(*param.second);
            result.emplace_back(param.first, clonedGenericType);
        }

        return result;
    }
}
