#include "TypeSignature.hpp"
#include <cstddef>
#include <sstream>

namespace types
{
    std::string TypeSignature::encodeType(const std::string& typeName)
    {
        // Already a canonical form — return as-is
        // This handles: int, float, bool, string, void, Object, ClassName, ClassName<T>, Type[], etc.
        return typeName;
    }

    std::string TypeSignature::encodeGenericParams(const std::vector<std::string>& params)
    {
        if (params.empty()) return "";

        std::string result = "<";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) result += ",";
            result += params[i];
        }
        result += ">";
        return result;
    }

    std::string TypeSignature::encodeClassSignature(
        const vm::bytecode::BytecodeProgram::ClassMetadata& classMeta)
    {
        std::string sig = classMeta.name;
        sig += encodeGenericParams(classMeta.genericParameters);

        // Encode parent class
        if (!classMeta.parentClassName.empty()) {
            sig += " extends " + classMeta.parentClassName;
        }

        // Encode implemented interfaces
        if (!classMeta.implementedInterfaces.empty()) {
            sig += " implements ";
            for (size_t i = 0; i < classMeta.implementedInterfaces.size(); ++i) {
                if (i > 0) sig += ",";
                sig += classMeta.implementedInterfaces[i];
            }
        }

        return sig;
    }

    std::string TypeSignature::encodeInterfaceSignature(
        const vm::bytecode::BytecodeProgram::InterfaceMetadata& interfaceMeta)
    {
        std::string sig = interfaceMeta.name;
        sig += encodeGenericParams(interfaceMeta.genericParameters);

        // Encode extended interfaces
        if (!interfaceMeta.extendsInterfaces.empty()) {
            sig += " extends ";
            for (size_t i = 0; i < interfaceMeta.extendsInterfaces.size(); ++i) {
                if (i > 0) sig += ",";
                sig += interfaceMeta.extendsInterfaces[i];
            }
        }

        return sig;
    }

    std::string TypeSignature::encodeFunctionSignature(
        const vm::bytecode::BytecodeProgram::FunctionMetadata& funcMeta)
    {
        std::string sig;

        // Generic type parameters
        if (!funcMeta.genericTypeParameters.empty()) {
            sig += "<";
            for (size_t i = 0; i < funcMeta.genericTypeParameters.size(); ++i) {
                if (i > 0) sig += ",";
                sig += funcMeta.genericTypeParameters[i];
            }
            sig += ">";
        }

        sig += funcMeta.name + "(";

        // Parameter types
        for (size_t i = 0; i < funcMeta.parameterTypes.size(); ++i) {
            if (i > 0) sig += ",";
            sig += funcMeta.parameterTypes[i];
        }

        sig += "):" + funcMeta.returnType;
        return sig;
    }

    std::string TypeSignature::encodeMethodSignature(
        const vm::bytecode::BytecodeProgram::MethodMetadata& methodMeta)
    {
        std::string sig;

        sig += methodMeta.name + "(";

        // Parameter types
        for (size_t i = 0; i < methodMeta.parameterTypes.size(); ++i) {
            if (i > 0) sig += ",";
            sig += methodMeta.parameterTypes[i];
        }

        sig += "):" + methodMeta.returnType;
        return sig;
    }

    bool TypeSignature::signaturesCompatible(const std::string& expected, const std::string& actual)
    {
        // Exact match is always compatible
        if (expected == actual) return true;

        // Both empty means compatible (void)
        if (expected.empty() && actual.empty()) return true;

        // For now, use structural equality
        // Future: handle covariant returns, contravariant params, generic substitution
        return false;
    }

    bool TypeSignature::isArrayType(const std::string& type)
    {
        return type.size() >= 2 && type.substr(type.size() - 2) == "[]";
    }

    bool TypeSignature::isLambdaType(const std::string& type)
    {
        return !type.empty() && type[0] == '(' && type.find(")->") != std::string::npos;
    }

    bool TypeSignature::isGenericType(const std::string& type)
    {
        return type.find('<') != std::string::npos;
    }

    std::string TypeSignature::extractBaseType(const std::string& type)
    {
        size_t pos = type.find('<');
        if (pos != std::string::npos) {
            return type.substr(0, pos);
        }
        return type;
    }
}
