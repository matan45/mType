#include "MethodDefinition.hpp"
#include <cstddef>
#include "../../parser/TypeParser.hpp"
#include "../../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include "../../types/TypeRegistry.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <stdexcept>
#include <cassert>

namespace runtimeTypes::klass
{
    ValueType MethodDefinition::resolveGenericParameter(size_t paramIndex, ValueType storedType) const
    {
        // Precondition assertion for debug builds
        assert(paramIndex < parameters.size() && "paramIndex must be within parameters bounds");

        // Check if we have unified type parameter information for this parameter
        if (paramIndex < unifiedParameters.size())
        {
            const auto& uParam = unifiedParameters[paramIndex].second;
            if (uParam)
            {
                if (uParam->isGenericParameter())
                {
                    // This is a direct generic type parameter (T, K, V) - resolve it
                    std::string typeParamName = uParam->getName();
                    auto it = typeSubstitutionMap.find(typeParamName);
                    if (it != typeSubstitutionMap.end())
                    {
                        return parser::TypeParser::stringToValueType(it->second);
                    }
                }
                else if (uParam->isParameterized())
                {
                    // This is a generic collection/object type (e.g., Set<T>, List<U>) - keep as OBJECT
                    return ValueType::OBJECT;
                }
            }
        }
        return storedType;
    }

    ValueType MethodDefinition::resolveFallbackMapping(size_t paramIndex) const
    {
        // Bounds check to prevent buffer overflow
        if (paramIndex >= parameters.size())
        {
            return ValueType::OBJECT;  // Safe fallback
        }

        // Count how many OBJECT parameters we've seen before this one
        size_t objectParamIndex = 0;
        // Loop only up to min(paramIndex, parameters.size()) to ensure safety
        const size_t safeLimit = std::min(paramIndex, parameters.size());
        for (size_t j = 0; j < safeLimit; ++j)
        {
            if (parameters[j].second == ValueType::OBJECT)
            {
                objectParamIndex++;
            }
        }

        // Map OBJECT parameters to type parameters in alphabetical order
        std::vector<std::string> sortedTypeParams;
        for (const auto& [key, value] : typeSubstitutionMap)
        {
            sortedTypeParams.push_back(key);
        }
        std::sort(sortedTypeParams.begin(), sortedTypeParams.end());

        // Use the nth type parameter for the nth OBJECT parameter
        if (objectParamIndex < sortedTypeParams.size())
        {
            auto it = typeSubstitutionMap.find(sortedTypeParams[objectParamIndex]);
            if (it != typeSubstitutionMap.end())
            {
                // With boxed types, we only resolve if the target type is also OBJECT
                // If target is primitive, keep as OBJECT (since boxed types are objects)
                value::ValueType targetType = parser::TypeParser::stringToValueType(it->second);
                if (targetType == ValueType::OBJECT)
                {
                    return targetType;
                }
                else
                {
                    // Target is primitive, but parameter is boxed object - keep as OBJECT
                    return ValueType::OBJECT;
                }
            }
        }
        return ValueType::OBJECT;
    }

    ValueType MethodDefinition::resolveParameterType(size_t paramIndex) const
    {
        // Bounds check for memory safety
        if (paramIndex >= parameters.size())
        {
            // Log warning in debug builds
            assert(false && "Attempting to resolve parameter type for out-of-bounds index");
            return ValueType::VOID;
        }

        // Get the stored parameter type (safe after bounds check)
        ValueType storedType = parameters[paramIndex].second;

        // If we have generic information and a substitution map, try to resolve the type
        if (isGeneric() && !typeSubstitutionMap.empty() && storedType == ValueType::OBJECT)
        {
            // Try to resolve using generic parameter information
            ValueType resolved = resolveGenericParameter(paramIndex, storedType);
            if (resolved != storedType)
            {
                return resolved;
            }

            // Fallback: Use position-based mapping for OBJECT parameters
            return resolveFallbackMapping(paramIndex);
        }

        // Fall back to the stored ValueType
        return storedType;
    }

    ValueType MethodDefinition::resolveParameterType(const std::string& paramName) const
    {
        // Find parameter by name
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i].first == paramName)
            {
                return resolveParameterType(i);
            }
        }
        return ValueType::VOID;
    }

    ValueType MethodDefinition::resolveReturnType() const
    {
        // For methods that return generic collections like Set<T>, the return type should be OBJECT
        if (isGeneric() && returnType == ValueType::OBJECT)
        {
            return ValueType::OBJECT;
        }

        // If we have generic information and a substitution map, resolve the return type
        if (isGeneric() && unifiedReturnType && unifiedReturnType->isGenericParameter())
        {
            std::string typeName = unifiedReturnType->getName();
            auto it = typeSubstitutionMap.find(typeName);
            if (it != typeSubstitutionMap.end())
            {
                return parser::TypeParser::stringToValueType(it->second);
            }
        }

        // Fall back to the stored ValueType
        return returnType;
    }

    void MethodDefinition::cleanupLambdaResources()
    {
        // Reset lambda implementation
        lambdaImplementation.reset();

        // Clear weak reference to lambda node
        lambdaNode.reset();

        // If the body is a LambdaInterfaceInvocationNode, clean it up
        if (body) {
            if (auto lambdaInvocationNode = std::dynamic_pointer_cast<ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                lambdaInvocationNode->cleanup();
            }
        }
    }

    bool MethodDefinition::needsLambdaCleanup() const
    {
        // Check if we have lambda resources that need cleanup
        if (hasLambdaImplementation() || hasLambdaNode()) {
            // Check if lambda node is expired
            if (!isLambdaNodeValid()) {
                return true;
            }

            // Check if lambda invocation node needs cleanup
            if (body) {
                if (auto lambdaInvocationNode = std::dynamic_pointer_cast<const ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                    return lambdaInvocationNode->needsCleanup();
                }
            }
        }

        return false;
    }

    std::string MethodDefinition::getLambdaLifecycleStatus() const
    {
        std::stringstream ss;
        ss << "MethodDefinition(name=" << getName();

        if (hasLambdaImplementation()) {
            ss << ", hasLambdaImpl=true";
        }

        if (hasLambdaNode()) {
            ss << ", hasLambdaNode=true, nodeValid=" << (isLambdaNodeValid() ? "true" : "false");
        }

        if (body) {
            if (auto lambdaInvocationNode = std::dynamic_pointer_cast<const ast::nodes::expressions::LambdaInterfaceInvocationNode>(body)) {
                ss << ", invocationNode=" << lambdaInvocationNode->getLambdaLifecycleStatus();
            }
        }

        ss << ")";
        return ss.str();
    }

    void MethodDefinition::validateParameterCounts(size_t paramCount, size_t genParamCount) const
    {
        if (genParamCount > 0 && paramCount != genParamCount)
        {
            std::stringstream ss;
            ss << "MethodDefinition '" << getName() << "': Parameter count mismatch. "
               << "Regular parameters: " << paramCount << ", "
               << "Generic parameters: " << genParamCount << ". "
               << "When generic parameter information is provided, counts must match.";
            throw std::invalid_argument(ss.str());
        }
    }

    void MethodDefinition::validateSubstitutionMap() const
    {
        // Only validate if we have both substitutions AND declared generic type parameters
        // If genericTypeParameters is empty, the substitution map was likely created at runtime
        // and we trust it's correct
        if (typeSubstitutionMap.empty() || genericTypeParameters.empty())
        {
            return; // Nothing to validate
        }

        // Build set of valid generic type parameter names
        std::unordered_set<std::string> validTypeParams;
        for (const auto& typeParam : genericTypeParameters)
        {
            validTypeParams.insert(typeParam.name);
        }

        // Validate each substitution key against declared parameters
        for (const auto& [key, value] : typeSubstitutionMap)
        {
            if (validTypeParams.find(key) == validTypeParams.end())
            {
                std::stringstream ss;
                ss << "MethodDefinition '" << getName() << "': Invalid type substitution key '" << key << "'. "
                   << "Key does not correspond to any declared generic type parameter. "
                   << "Valid parameters: [";

                bool first = true;
                for (const auto& param : genericTypeParameters)
                {
                    if (!first) ss << ", ";
                    ss << param.name;
                    first = false;
                }
                ss << "]";
                throw std::invalid_argument(ss.str());
            }
        }
    }

    void MethodDefinition::validateGenericTypeRecursive(const ::types::UnifiedTypePtr& type, const std::string& context) const
    {
        if (!type)
        {
            return; // Nothing to validate
        }

        // If the type is parameterized (e.g., HashSet<T>, Map<K,V>), validate type arguments
        if (type->isParameterized())
        {
            const auto& typeArgs = type->getTypeArguments();
            for (size_t i = 0; i < typeArgs.size(); ++i)
            {
                std::stringstream contextStr;
                contextStr << context << " type argument " << i;
                validateGenericTypeRecursive(typeArgs[i], contextStr.str());
            }
        }
        // If it's a generic parameter (could be T, K, V, etc.)
        else if (type->isGenericParameter())
        {
            std::string typeName = type->getName();

            // Check if this is actually a generic type parameter (T, E, K, V)
            // vs a class name (String, HashSet, etc.)
            // Generic parameters are single uppercase letters
            if (types::TypeRegistry::isGenericParameter(typeName))
            {
                // This is a true generic parameter - validate it's declared
                bool found = false;
                for (const auto& typeParam : genericTypeParameters)
                {
                    if (typeParam.name == typeName)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    std::stringstream ss;
                    ss << "MethodDefinition '" << getName() << "': Generic " << context
                       << " uses undeclared type parameter '" << typeName << "'. "
                       << "Valid parameters: [";

                    bool first = true;
                    for (const auto& param : genericTypeParameters)
                    {
                        if (!first) ss << ", ";
                        ss << param.name;
                        first = false;
                    }
                    ss << "]";
                    throw std::invalid_argument(ss.str());
                }
            }
            // else: It's a class name (String, HashSet, etc.) - no validation needed
        }
        // Concrete ValueType types (int, float, bool, etc.) don't need validation
    }

    void MethodDefinition::validateGenericInvariants() const
    {
        // Validate type substitution map
        validateSubstitutionMap();

        // If we have unified parameters, ensure consistency
        if (!unifiedParameters.empty())
        {
            // Check that all unified parameters have valid types
            for (size_t i = 0; i < unifiedParameters.size(); ++i)
            {
                const auto& [paramName, uType] = unifiedParameters[i];

                if (!uType)
                {
                    std::stringstream ss;
                    ss << "MethodDefinition '" << getName() << "': Unified parameter at index " << i
                       << " ('" << paramName << "') has null type pointer.";
                    throw std::invalid_argument(ss.str());
                }
            }
        }

        // Note: We allow typeSubstitutionMap without genericTypeParameters
        // This can happen when generic methods are instantiated at runtime
        // The substitution map is created dynamically based on usage

        // Validate unified return type if present
        // Only validate if we have formal generic type parameter declarations
        if (unifiedReturnType && !genericTypeParameters.empty())
        {
            validateGenericTypeRecursive(unifiedReturnType, "return type");
        }
    }

    bool MethodDefinition::hasAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<ast::nodes::annotations::AnnotationNode> MethodDefinition::getAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return annotation;
            }
        }
        return nullptr;
    }

    std::string MethodDefinition::getTypeSignature() const
    {
        // IMPORTANT: Skip "this" parameter for instance methods (it's added implicitly)
        // Instance methods have "this" as first parameter, static methods don't
        size_t startIndex = isStaticMethod ? 0 : 1;

        if (parameters.size() <= startIndex) {
            return "";
        }

        std::string signature;
        for (size_t i = startIndex; i < parameters.size(); ++i) {
            if (i > startIndex) signature += ",";

            const auto& paramType = parameters[i].second;

            // Use class name if it's an object, otherwise use basic type display name
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                signature += paramType.className.value();
            } else {
                signature += ::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType);
            }
        }
        return signature;
    }
}
