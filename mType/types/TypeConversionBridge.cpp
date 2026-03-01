#include "TypeConversionBridge.hpp"
#include <sstream>

namespace types
{
    UnifiedTypePtr TypeConversionBridge::toUnifiedType(
        const std::shared_ptr<ast::GenericType>& genericType)
    {
        if (!genericType)
        {
            return nullptr;
        }

        // Handle generic type parameters (T, K, V, etc.)
        if (genericType->isGenericParameter())
        {
            std::string name = genericType->getGenericName();

            // Check if this is Array<T> (special case for arrays)
            if (name == "Array" && genericType->isParameterized())
            {
                const auto& typeArgs = genericType->getTypeArguments();
                if (!typeArgs.empty())
                {
                    auto elementType = toUnifiedType(typeArgs[0]);
                    return UnifiedType::arrayOf(elementType);
                }
            }

            // Check if it's parameterized (e.g., Container<T>)
            if (genericType->isParameterized())
            {
                std::vector<UnifiedTypePtr> typeArgs;
                for (const auto& arg : genericType->getTypeArguments())
                {
                    typeArgs.push_back(toUnifiedType(arg));
                }
                return UnifiedType::classType(name, std::move(typeArgs));
            }

            // Simple generic parameter (T, E, etc.)
            return UnifiedType::genericParam(name);
        }

        // Handle concrete types
        value::ValueType baseType = genericType->getConcreteType();

        switch (baseType)
        {
            case value::ValueType::INT:
                return UnifiedType::primitive(value::ValueType::INT);
            case value::ValueType::FLOAT:
                return UnifiedType::primitive(value::ValueType::FLOAT);
            case value::ValueType::BOOL:
                return UnifiedType::primitive(value::ValueType::BOOL);
            case value::ValueType::STRING:
                return UnifiedType::primitive(value::ValueType::STRING);
            case value::ValueType::VOID:
                return UnifiedType::voidType();
            case value::ValueType::NULL_TYPE:
                return UnifiedType::nullType();
            case value::ValueType::LAMBDA:
                return UnifiedType::lambdaType();
            case value::ValueType::ARRAY:
            {
                if (genericType->isParameterized())
                {
                    const auto& typeArgs = genericType->getTypeArguments();
                    if (!typeArgs.empty())
                    {
                        auto elementType = toUnifiedType(typeArgs[0]);
                        return UnifiedType::arrayOf(elementType);
                    }
                }
                return UnifiedType::arrayOf(UnifiedType::classType("Object"));
            }
            case value::ValueType::OBJECT:
            default:
            {
                std::string className = genericType->getBaseTypeName();

                if (genericType->isParameterized())
                {
                    std::vector<UnifiedTypePtr> typeArgs;
                    for (const auto& arg : genericType->getTypeArguments())
                    {
                        typeArgs.push_back(toUnifiedType(arg));
                    }
                    return UnifiedType::classType(className, std::move(typeArgs));
                }

                return UnifiedType::classType(className);
            }
        }
    }

    std::vector<TypeConstraint> TypeConversionBridge::convertConstraints(
        const ast::GenericTypeParameter& param)
    {
        std::vector<TypeConstraint> result;

        for (const auto& constraintStr : param.constraints)
        {
            // Default to Implements for interface constraints
            // Could be enhanced to detect "extends" keyword in constraint string
            result.push_back(parseConstraintString(constraintStr, TypeConstraint::Kind::Implements));
        }

        return result;
    }

    TypeConstraint TypeConversionBridge::parseConstraintString(
        const std::string& constraintStr,
        TypeConstraint::Kind kind)
    {
        UnifiedTypePtr boundType = parseConstraintType(constraintStr);
        return TypeConstraint(kind, boundType);
    }

    UnifiedTypePtr TypeConversionBridge::toUnifiedTypeParam(
        const ast::GenericTypeParameter& param)
    {
        std::vector<TypeConstraint> constraints = convertConstraints(param);
        return UnifiedType::genericParam(param.name, std::move(constraints));
    }

    std::vector<UnifiedTypePtr> TypeConversionBridge::convertGenericParams(
        const std::vector<ast::GenericTypeParameter>& params)
    {
        std::vector<UnifiedTypePtr> result;
        result.reserve(params.size());

        for (const auto& param : params)
        {
            result.push_back(toUnifiedTypeParam(param));
        }

        return result;
    }

    std::vector<std::string> TypeConversionBridge::extractParamNames(
        const std::vector<ast::GenericTypeParameter>& params)
    {
        std::vector<std::string> result;
        result.reserve(params.size());

        for (const auto& param : params)
        {
            result.push_back(param.name);
        }

        return result;
    }

    UnifiedTypePtr TypeConversionBridge::parseConstraintType(const std::string& constraintStr)
    {
        if (constraintStr.empty())
        {
            return nullptr;
        }

        // Find generic parameters
        size_t anglePos = constraintStr.find('<');
        if (anglePos == std::string::npos)
        {
            // Simple type name without generics
            return UnifiedType::interfaceType(constraintStr);
        }

        // Parse generic type: "Comparable<T>"
        std::string baseName = constraintStr.substr(0, anglePos);
        std::string argsStr = constraintStr.substr(anglePos + 1);

        // Remove trailing '>'
        if (!argsStr.empty() && argsStr.back() == '>')
        {
            argsStr.pop_back();
        }

        // Parse type arguments (simplified - handles single arg and comma-separated)
        std::vector<UnifiedTypePtr> typeArgs;
        std::string currentArg;
        int depth = 0;

        for (char c : argsStr)
        {
            if (c == '<')
            {
                depth++;
                currentArg += c;
            }
            else if (c == '>')
            {
                depth--;
                currentArg += c;
            }
            else if (c == ',' && depth == 0)
            {
                // End of current argument
                if (!currentArg.empty())
                {
                    // Trim whitespace
                    size_t start = currentArg.find_first_not_of(" \t");
                    size_t end = currentArg.find_last_not_of(" \t");
                    if (start != std::string::npos)
                    {
                        currentArg = currentArg.substr(start, end - start + 1);
                    }
                    typeArgs.push_back(parseConstraintType(currentArg));
                    currentArg.clear();
                }
            }
            else
            {
                currentArg += c;
            }
        }

        // Don't forget the last argument
        if (!currentArg.empty())
        {
            size_t start = currentArg.find_first_not_of(" \t");
            size_t end = currentArg.find_last_not_of(" \t");
            if (start != std::string::npos)
            {
                currentArg = currentArg.substr(start, end - start + 1);
            }
            typeArgs.push_back(parseConstraintType(currentArg));
        }

        return UnifiedType::interfaceType(baseName, std::move(typeArgs));
    }

    std::string TypeConversionBridge::extractBaseTypeName(const std::string& typeName)
    {
        size_t anglePos = typeName.find('<');
        if (anglePos != std::string::npos)
        {
            return typeName.substr(0, anglePos);
        }
        return typeName;
    }
}
