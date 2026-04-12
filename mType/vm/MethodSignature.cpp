#include "MethodSignature.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include <sstream>

namespace vm {

    MethodSignature MethodSignature::fromMethodDefinition(
        const runtimeTypes::klass::MethodDefinition* method)
    {
        if (!method) {
            return MethodSignature("", {});
        }

        std::string name = method->getName();
        std::vector<std::string> paramTypes;

        // Use unifiedParameters which does NOT include 'this' for instance methods.
        // For bytecode-deserialized methods unifiedParameters is empty — fall back
        // to the regular parameter list, skipping the prepended 'this'.
        const auto& uParams = method->getUnifiedParameters();
        if (!uParams.empty())
        {
            paramTypes.reserve(uParams.size());
            for (const auto& param : uParams) {
                paramTypes.push_back(param.second ? param.second->toString() : "void");
            }
        }
        else
        {
            const auto& params = method->getParameters();
            for (const auto& [pname, ptype] : params) {
                if (pname == "this") continue;
                paramTypes.push_back(ptype.toString());
            }
        }

        return MethodSignature(name, paramTypes);
    }

    MethodSignature MethodSignature::fromMethodNode(
        const ast::nodes::classes::MethodNode* node,
        bool isStatic)
    {
        if (!node) {
            return MethodSignature("", {});
        }

        std::string name = node->getName();
        std::vector<std::string> paramTypes;

        // Get generic parameters from AST node
        // These do NOT include 'this' - 'this' is added implicitly during compilation
        const auto& genericParams = node->getGenericParameters();
        paramTypes.reserve(genericParams.size());

        for (const auto& param : genericParams) {
            paramTypes.push_back(param.second->toString());
        }

        return MethodSignature(name, paramTypes);
    }

    MethodSignature MethodSignature::fromNameAndCount(
        const std::string& name,
        size_t paramCount)
    {
        // Create generic parameter type names for signature matching
        std::vector<std::string> paramTypes;
        paramTypes.reserve(paramCount);

        for (size_t i = 0; i < paramCount; ++i) {
            paramTypes.push_back("");  // Empty string matches any type for count-based lookup
        }

        return MethodSignature(name, paramTypes);
    }

    std::string MethodSignature::toMangledName(const std::string& className, bool isStatic) const
    {
        std::ostringstream oss;
        oss << className << "::" << methodName;

        // Add type signature if there are parameters
        if (!parameterTypes.empty()) {
            oss << "/";
            for (size_t i = 0; i < parameterTypes.size(); ++i) {
                if (i > 0) oss << ",";
                oss << parameterTypes[i];
            }
        }

        // Add $static suffix for static methods
        if (isStatic) {
            oss << "$static";
        }

        return oss.str();
    }

    size_t MethodSignature::hash() const
    {
        // Combine hash of method name and parameter types
        size_t h = std::hash<std::string>{}(methodName);

        for (const auto& paramType : parameterTypes) {
            // Combine hashes using a common technique
            h ^= std::hash<std::string>{}(paramType) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

        return h;
    }

    std::string MethodSignature::toString() const
    {
        std::ostringstream oss;
        oss << methodName << "(";

        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << parameterTypes[i];
        }

        oss << ")";
        return oss.str();
    }

} // namespace vm
