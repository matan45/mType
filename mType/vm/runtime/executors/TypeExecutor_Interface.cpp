#include "TypeExecutor.hpp"
#include <sstream>
#include "../utils/NullCheckUtils.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/InterfaceDefinition.hpp"

namespace vm::runtime
{
    bool TypeExecutor::checkInterfaceHierarchy(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited,
        environment::Environment* env
    ) {
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        auto stripGenerics = [](const std::string& name) -> std::string {
            return ::types::TypeConversionUtils::extractBaseTypeName(name);
        };

        std::string interfaceBaseName = stripGenerics(interfaceName);
        std::string targetBaseName = stripGenerics(targetInterface);

        std::string interfaceTypeParams = "";
        size_t ifaceGenericStart = interfaceName.find('<');
        if (ifaceGenericStart != std::string::npos) {
            interfaceTypeParams = interfaceName.substr(ifaceGenericStart);
        }

        std::string targetTypeParams = "";
        size_t targetGenericStart = targetInterface.find('<');
        if (targetGenericStart != std::string::npos) {
            targetTypeParams = targetInterface.substr(targetGenericStart);
        }

        if (interfaceName == targetInterface ||
            (interfaceBaseName == targetBaseName && targetTypeParams.empty())) {
            return true;
        }
        if (interfaceBaseName == targetBaseName && !targetTypeParams.empty()) {
            return (interfaceTypeParams == targetTypeParams);
        }

        auto interfaceDef = env ? env->findInterface(interfaceName) : nullptr;
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                if (checkInterfaceHierarchy(extendedInterface, targetInterface, visited, env)) {
                    return true;
                }
            }
        }

        return false;
    }

    // MYT-44: parameterized interface-hierarchy walk. `interfaceName` is
    // already substituted (e.g. "SortedList<Int>") — caller resolved the
    // receiver's bindings before invoking us. At each recursion step we
    // look up the interface's definition, rebuild the binding map using
    // its OWN declared parameters (which may use different letters than
    // the caller), then substitute its extended interfaces and recurse.
    //
    // Split from the raw-mode overload because the two modes have
    // materially different recursion contracts (bindings must travel
    // alongside the name in parameterized mode) and MYT-44 explicitly
    // does not want to risk regressing the raw path.
    bool TypeExecutor::checkInterfaceHierarchyParam(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited,
        const std::unordered_map<std::string, std::string>& /*bindings*/,
        environment::Environment* env
    ) {
        // Use the full substituted name as the visited key so two different
        // parameterized views of the same template (SortedList<Int> vs
        // SortedList<String>) reached through different paths don't collide.
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        if (interfaceName == targetInterface) {
            return true;
        }

        // Split into base + params so we can look up InterfaceDefinition by
        // raw base name AND feed substituted args into rebindForInterface
        // for the recursion step.
        TypeComponents currentComp = extractTypeComponents(interfaceName);

        auto interfaceDef = env ? env->findInterface(currentComp.baseName) : nullptr;
        if (!interfaceDef) {
            return false;
        }

        // Rebuild bindings using the interface's own declared params. When
        // arity matches, new bindings keyed by the interface's param names.
        // When it doesn't (raw extends or malformed), rebindForInterface
        // returns empty; substitutions fall back to verbatim — correct for
        // raw targets but fails parameterized match, as desired.
        auto newBindings = rebindForInterface(interfaceDef.get(), currentComp.typeParams);

        const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
        for (const auto& extendedInterface : extendedInterfaces) {
            std::string substituted = substituteTypeExpression(extendedInterface, newBindings);
            if (checkInterfaceHierarchyParam(substituted, targetInterface, visited, newBindings, env)) {
                return true;
            }
        }

        return false;
    }

    std::string TypeExecutor::valueToString(const value::Value& val) {
        if (value::isInt(val)) {
            return std::to_string(value::asInt(val));
        }
        if (value::isFloat(val)) {
            std::ostringstream oss;
            oss << value::asFloat(val);
            return oss.str();
        }
        if (value::isBool(val)) {
            return value::asBool(val) ? "true" : "false";
        }
        // MYT-317: SSO-aware stringify.
        if (value::isAnyString(val)) {
            return std::string(value::asStringView(val));
        }
        if (utils::isNullValue(val)) {
            return "null";
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->hasField("value") && obj->getFieldCount() == 1) {
                return valueToString(obj->getFieldValue("value"));
            }
            return obj ? "<" + obj->getClassName() + ">" : "null";
        }
        // MYT-208: accept STACK_OBJECT alongside OBJECT — `(string) p` on a
        // stack-promoted local should produce the wrapper's "value" field
        // when present, mirroring the OBJECT-tag fallback.
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getField("value")) {
                return valueToString(obj->getFieldValue("value"));
            }
        }
        return "<object>";
    }

    std::string TypeExecutor::buildParameterizedName(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        const std::unordered_map<std::string, std::string>& bindings) {
        const auto& declaredParams = classDef->getGenericParameters();
        if (declaredParams.empty()) {
            return classDef->getName();
        }

        // Partial bindings → type-erased fallback (raw base name). Matches
        // backward-compat behavior for instances created via `new Box(...)`
        // without explicit type arguments.
        std::vector<std::string> args;
        args.reserve(declaredParams.size());
        for (const auto& p : declaredParams) {
            auto it = bindings.find(p.name);
            if (it == bindings.end() || it->second.empty()) {
                return classDef->getName();
            }
            args.push_back(it->second);
        }

        // Spacing MUST match ast::GenericType::toString() exactly ("<", ", ",
        // ">") so compiler-emitted target string and runtime-reconstructed
        // receiver string are comparable via raw string equality.
        std::string out = classDef->getName();
        out += "<";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) out += ", ";
            out += args[i];
        }
        out += ">";
        return out;
    }

    std::string TypeExecutor::reconstructObjectFullType(
        const runtimeTypes::klass::ObjectInstance* obj) {
        if (!obj) {
            return "";
        }
        return buildParameterizedName(obj->getClassDefinition(),
                                      obj->getGenericTypeBindings());
    }

    std::string TypeExecutor::reconstructValueObjectFullType(
        const std::shared_ptr<value::ValueObject>& obj) {
        if (!obj) {
            return "";
        }
        return buildParameterizedName(obj->getClassDefinition(),
                                      obj->getGenericTypeBindings());
    }

    std::string TypeExecutor::reconstructMultiArrayTypeName(const value::Value& val) {
        const value::Value* defaultPtr = nullptr;
        size_t rank = 0;
        if (value::isFlatMultiArray(val)) {
            const auto& arr = value::asFlatMultiArray(val);
            defaultPtr = &arr->getDefaultValue();
            rank = arr->getRank();
        } else if (value::isSparseMultiArray(val)) {
            const auto& arr = value::asSparseMultiArray(val);
            defaultPtr = &arr->getDefaultValue();
            rank = arr->getRank();
        }
        if (!defaultPtr || rank == 0) {
            return "";
        }
        const char* elemName = nullptr;
        switch (defaultPtr->tag()) {
            case value::ValueType::INT:    elemName = "int";    break;
            case value::ValueType::FLOAT:  elemName = "float";  break;
            case value::ValueType::BOOL:   elemName = "bool";   break;
            case value::ValueType::STRING: elemName = "string"; break;
            default: break;
        }
        if (!elemName) {
            return "";
        }
        std::string fullName(elemName);
        fullName.reserve(fullName.size() + rank * 2);
        for (size_t i = 0; i < rank; ++i) {
            fullName += "[]";
        }
        return fullName;
    }
}
