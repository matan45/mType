#include "SignatureUtils.hpp"
#include <cstddef>
#include "../../types/TypeConversionUtils.hpp"
#include <sstream>
#include <algorithm>

namespace runtimeTypes::klass
{
    void SignatureUtils::ensureStaticSuffix(std::string& name)
    {
        // find() avoids a substring allocation and handles the
        // suffix-anywhere-in-name case (pedantic: the suffix should only
        // ever appear at the end, but the prior inline checks also used
        // find() so we keep identical semantics).
        if (name.find(STATIC_SUFFIX) == std::string::npos) {
            name += STATIC_SUFFIX;
        }
    }

    std::string SignatureUtils::getTypeName(const value::ParameterType& paramType)
    {
        // MYT-282: route through the ParameterType overload — preserves
        // `int[]` for ARRAY-tag params and falls through to class /
        // interface / primitive names for everything else.
        return types::TypeConversionUtils::getTypeDisplayName(paramType);
    }

    std::string SignatureUtils::generateTypeSignature(
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        if (parameters.empty()) {
            return "";
        }

        std::vector<std::string> typeNames;
        typeNames.reserve(parameters.size());

        for (const auto& [paramName, paramType] : parameters) {
            typeNames.push_back(getTypeName(paramType));
        }

        return join(typeNames, ",");
    }

    std::string SignatureUtils::generateTypeSignature(
        const std::vector<value::ValueType>& paramTypes)
    {
        if (paramTypes.empty()) {
            return "";
        }

        std::vector<std::string> typeNames;
        typeNames.reserve(paramTypes.size());

        for (const auto& type : paramTypes) {
            typeNames.push_back(types::TypeConversionUtils::getTypeDisplayName(type));
        }

        return join(typeNames, ",");
    }

    std::string SignatureUtils::generateTypeSignatureFromNames(
        const std::vector<std::string>& typeNames)
    {
        if (typeNames.empty()) {
            return "";
        }

        return join(typeNames, ",");
    }

    std::string SignatureUtils::buildQualifiedName(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        std::string typeSignature = generateTypeSignature(parameters);
        return buildQualifiedName(className, methodName, typeSignature);
    }

    std::string SignatureUtils::buildQualifiedName(
        const std::string& className,
        const std::string& methodName,
        const std::string& typeSignature)
    {
        if (typeSignature.empty()) {
            return className + "::" + methodName;
        }
        return className + "::" + methodName + "/" + typeSignature;
    }

    std::string SignatureUtils::buildFunctionName(
        const std::string& functionName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        std::string typeSignature = generateTypeSignature(parameters);
        return buildFunctionName(functionName, typeSignature);
    }

    std::string SignatureUtils::buildFunctionName(
        const std::string& functionName,
        const std::string& typeSignature)
    {
        if (typeSignature.empty()) {
            return functionName;
        }
        return functionName + "/" + typeSignature;
    }

    bool SignatureUtils::parseQualifiedName(
        const std::string& qualifiedName,
        std::string& className,
        std::string& methodName,
        std::string& typeSignature)
    {
        // Format: ClassName::methodName/typeSignature

        // Find "::" separator
        size_t classDelim = qualifiedName.find("::");
        if (classDelim == std::string::npos) {
            return false;
        }

        className = qualifiedName.substr(0, classDelim);

        // Find "/" separator
        size_t sigDelim = qualifiedName.find('/', classDelim + 2);
        if (sigDelim == std::string::npos) {
            // No signature part - method name is rest of string
            methodName = qualifiedName.substr(classDelim + 2);
            typeSignature = "";
        } else {
            methodName = qualifiedName.substr(classDelim + 2, sigDelim - classDelim - 2);
            typeSignature = qualifiedName.substr(sigDelim + 1);
        }

        return true;
    }

    bool SignatureUtils::parseFunctionName(
        const std::string& mangledName,
        std::string& functionName,
        std::string& typeSignature)
    {
        // Format: functionName/typeSignature

        size_t sigDelim = mangledName.find('/');
        if (sigDelim == std::string::npos) {
            // No signature part
            functionName = mangledName;
            typeSignature = "";
        } else {
            functionName = mangledName.substr(0, sigDelim);
            typeSignature = mangledName.substr(sigDelim + 1);
        }

        return true;
    }

    std::vector<std::string> SignatureUtils::parseTypeSignature(
        const std::string& typeSignature)
    {
        if (typeSignature.empty()) {
            return {};
        }

        return split(typeSignature, ",");
    }

    std::string SignatureUtils::extractSimpleName(const std::string& name)
    {
        // Handle qualified names: "ClassName::methodName/signature" -> "methodName"
        size_t classDelim = name.find("::");
        size_t sigDelim = name.find('/');

        if (classDelim != std::string::npos) {
            // Has class qualifier
            if (sigDelim != std::string::npos && sigDelim > classDelim) {
                // "ClassName::methodName/signature" -> "methodName"
                return name.substr(classDelim + 2, sigDelim - classDelim - 2);
            } else {
                // "ClassName::methodName" -> "methodName"
                return name.substr(classDelim + 2);
            }
        } else if (sigDelim != std::string::npos) {
            // "functionName/signature" -> "functionName"
            return name.substr(0, sigDelim);
        } else {
            // Simple name
            return name;
        }
    }

    bool SignatureUtils::hasSignature(const std::string& name)
    {
        return name.find('/') != std::string::npos;
    }

    std::string SignatureUtils::join(const std::vector<std::string>& elements,
                                     const std::string& delimiter)
    {
        if (elements.empty()) {
            return "";
        }

        std::ostringstream oss;
        oss << elements[0];
        for (size_t i = 1; i < elements.size(); ++i) {
            oss << delimiter << elements[i];
        }
        return oss.str();
    }

    std::vector<std::string> SignatureUtils::split(const std::string& str,
                                                    const std::string& delimiter)
    {
        std::vector<std::string> result;
        if (str.empty()) {
            return result;
        }

        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }

        result.push_back(str.substr(start));
        return result;
    }

} // namespace runtimeTypes::klass
