#include "TypeExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/UserException.hpp"
#include <sstream>
#include <iostream>
namespace vm::runtime
{
    TypeExecutor::TypeExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void TypeExecutor::handleInstanceof(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get target type name from constant pool
        const std::string& targetTypeName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop value to check
        value::Value val = context.stackManager->pop();

        bool result = false;

        // Check if it's a primitive type
        if (checkInstanceofPrimitive(val, targetTypeName)) {
            result = true;
        } else {
            // Check if it's an object type
            result = checkInstanceofObject(val, targetTypeName);
        }

        // Push boolean result onto stack
        context.stackManager->push(result);
    }

    void TypeExecutor::handleCast(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get target type name from constant pool
        const std::string& targetTypeName = context.program->getConstantPool().getString(instr.operands[0]);

        // Pop value to cast
        value::Value val = context.stackManager->pop();

        // Perform casting based on target type
        if (targetTypeName == "Int" || targetTypeName == "int") {
            context.stackManager->push(castToInt(val));
        }
        else if (targetTypeName == "Float" || targetTypeName == "float") {
            context.stackManager->push(castToFloat(val));
        }
        else if (targetTypeName == "String" || targetTypeName == "string") {
            context.stackManager->push(castToString(val));
        }
        else if (targetTypeName == "Bool" || targetTypeName == "bool") {
            context.stackManager->push(castToBool(val));
        }
        else {
            context.stackManager->push(castToObject(val, targetTypeName));
        }
    }

    bool TypeExecutor::checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName) {
        if (targetTypeName == "Int" || targetTypeName == "int") {
            return std::holds_alternative<int64_t>(val);
        }
        if (targetTypeName == "Float" || targetTypeName == "float") {
            return std::holds_alternative<double>(val);
        }
        if (targetTypeName == "Bool" || targetTypeName == "bool") {
            return std::holds_alternative<bool>(val);
        }
        if (targetTypeName == "String" || targetTypeName == "string") {
            return std::holds_alternative<std::string>(val) || std::holds_alternative<value::InternedString>(val);
        }
        return false;
    }

    bool TypeExecutor::checkInstanceofObject(const value::Value& val, const std::string& targetTypeName) {
        // Object type check
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

            if (obj) {
                auto classDef = obj->getClassDefinition();
                std::string className = classDef->getName();

                // Extract base class name from generic instantiation (e.g., "Box<Int>" -> "Box")
                std::string baseClassName = ::types::TypeConversionUtils::extractBaseTypeName(className);
                std::string baseTargetName = ::types::TypeConversionUtils::extractBaseTypeName(targetTypeName);

                // Check if object's class matches target type (exact or base generic match)
                bool result = (className == targetTypeName) || (baseClassName == baseTargetName);

                // If not exact match, check inheritance hierarchy
                if (!result) {
                    result = classDef->isSubclassOf(targetTypeName);

                    // Also check with base names for generic types
                    if (!result && baseTargetName != targetTypeName) {
                        result = classDef->isSubclassOf(baseTargetName);
                    }
                }

                // Also check if object implements an interface with that name
                // IMPORTANT: Must check interface hierarchy recursively
                // AND check parent classes' interfaces too
                if (!result) {
                    // Walk up the inheritance chain checking interfaces at each level
                    auto currentClass = classDef;
                    while (currentClass && !result) {
                        const auto& interfaces = currentClass->getImplementedInterfaces();
                        for (const auto& iface : interfaces) {
                            // Extract base interface name for comparison
                            std::string baseIfaceName = ::types::TypeConversionUtils::extractBaseTypeName(iface);

                            if (iface == targetTypeName || baseIfaceName == baseTargetName) {
                                result = true;
                                break;
                            }

                            // Check if this interface extends the target interface (interface inheritance)
                            std::unordered_set<std::string> visited;
                            if (checkInterfaceHierarchy(iface, targetTypeName, visited)) {
                                result = true;
                                break;
                            }
                        }

                        // Move to parent class
                        currentClass = currentClass->getParentClass();
                    }
                }

                return result;
            } else {
                return false; // null is not instance of any type
            }
        }
        // ValueObject type check (value classes)
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj) {
                auto classDef = obj->getClassDefinition();
                std::string className = classDef->getName();
                std::string baseClassName = ::types::TypeConversionUtils::extractBaseTypeName(className);
                std::string baseTargetName = ::types::TypeConversionUtils::extractBaseTypeName(targetTypeName);

                bool result = (className == targetTypeName) || (baseClassName == baseTargetName);

                // Check implemented interfaces
                if (!result) {
                    const auto& interfaces = classDef->getImplementedInterfaces();
                    for (const auto& iface : interfaces) {
                        std::string baseIfaceName = ::types::TypeConversionUtils::extractBaseTypeName(iface);
                        if (iface == targetTypeName || baseIfaceName == baseTargetName) {
                            result = true;
                            break;
                        }
                        std::unordered_set<std::string> visited;
                        if (checkInterfaceHierarchy(iface, targetTypeName, visited)) {
                            result = true;
                            break;
                        }
                    }
                }

                return result;
            }
            return false;
        }
        else if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
            return false; // null is not instance of any type
        }
        else {
            return false; // primitive values are not instances of object types
        }
    }

    value::Value TypeExecutor::castToInt(const value::Value& val) {
        if (std::holds_alternative<int64_t>(val)) {
            return val; // Already int
        }
        else if (std::holds_alternative<double>(val)) {
            return static_cast<int64_t>(std::get<double>(val));
        }
        else if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? static_cast<int64_t>(1) : static_cast<int64_t>(0);
        }
        else if (std::holds_alternative<std::string>(val)) {
            try {
                return std::stoll(std::get<std::string>(val));
            } catch (...) {
                throwCastError("Cannot cast string to int: " + std::get<std::string>(val));
            }
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj && obj->getClassDefinition()->getName() == "Int") {
                // Return the Int object itself (it's already the right type)
                return val;
            }
            throwCastError("Cannot cast object to int");
        }
        else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj && obj->getClassName() == "Int") {
                return val;
            }
            throwCastError("Cannot cast value object to int");
        }
        else {
            throwCastError("Cannot cast to int from this type");
        }
    }

    value::Value TypeExecutor::castToFloat(const value::Value& val) {
        if (std::holds_alternative<double>(val)) {
            return val; // Already float
        }
        else if (std::holds_alternative<int64_t>(val)) {
            return static_cast<double>(std::get<int64_t>(val));
        }
        else if (std::holds_alternative<std::string>(val)) {
            try {
                return std::stod(std::get<std::string>(val));
            } catch (...) {
                throwCastError("Cannot cast string to float: " + std::get<std::string>(val));
            }
        }
        else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj && obj->getClassName() == "Float") {
                return val;
            }
            throwCastError("Cannot cast value object to float");
        }
        else {
            throwCastError("Cannot cast to float from this type");
        }
    }

    value::Value TypeExecutor::castToString(const value::Value& val) {
        return valueToString(val);
    }

    value::Value TypeExecutor::castToBool(const value::Value& val) {
        if (std::holds_alternative<bool>(val)) {
            return val; // Already bool
        }
        else if (std::holds_alternative<int64_t>(val)) {
            return std::get<int64_t>(val) != 0;
        }
        else if (std::holds_alternative<double>(val)) {
            return std::get<double>(val) != 0.0;
        }
        else if (std::holds_alternative<std::string>(val)) {
            const std::string& str = std::get<std::string>(val);
            // Non-empty strings are true
            return !str.empty();
        }
        else if (std::holds_alternative<value::InternedString>(val)) {
            const value::InternedString& str = std::get<value::InternedString>(val);
            // Non-empty strings are true
            return str.length() > 0;
        }
        else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj && obj->getClassName() == "Bool") {
                return val;
            }
            throwCastError("Cannot cast value object to bool");
        }
        else {
            throwCastError("Cannot cast to bool from this type");
        }
    }

    TypeExecutor::TypeComponents TypeExecutor::extractTypeComponents(const std::string& typeName) {
        TypeComponents components;
        components.baseName = typeName;
        components.typeParams = "";

        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            components.baseName = typeName.substr(0, genericStart);
            components.typeParams = typeName.substr(genericStart); // includes <>
        }

        return components;
    }

    bool TypeExecutor::checkExactMatch(const std::string& className, const std::string& targetTypeName,
                                       const TypeComponents& classComp, const TypeComponents& targetComp) {
        // 1. Exact match (including generic type parameters)
        if (className == targetTypeName) {
            return true;
        }
        // 2. Base class match without type parameters (e.g., Box<Int> -> Box)
        if (classComp.baseName == targetComp.baseName && targetComp.typeParams.empty()) {
            return true;
        }
        // 3. Type erasure compatibility: allow casting from erased type to parameterized type
        // (e.g., Box -> Box<Int>). This happens because mType uses type erasure at runtime.
        if (classComp.baseName == targetComp.baseName && classComp.typeParams.empty()) {
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkUpcastMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                        const std::string& targetTypeName, const TypeComponents& targetComp,
                                        const std::string& baseClassName, const std::string& classTypeParams) {
        // 3. Upcast - object is subclass of target type
        if (classDef->isSubclassOf(targetTypeName)) {
            return true;
        }
        // 4. Upcast with base names (for generic types)
        if (classDef->isSubclassOf(targetComp.baseName)) {
            // Check if type parameters match (if target has type params)
            if (!targetComp.typeParams.empty() && !classTypeParams.empty()) {
                // Type parameters must match for generic upcast
                return (classTypeParams == targetComp.typeParams);
            }
            // No type params on target, allow upcast to generic base
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkDowncastMatch(const std::string& baseClassName, const std::string& baseTargetName,
                                          const std::string& classTypeParams, const std::string& targetTypeParams) {
        // Get target class from registry to check if it's a subclass
        auto targetClass = context.environment->findClass(baseTargetName);
        if (!targetClass) {
            return false;
        }

        // Check if target class is a subclass of the object's actual class
        if (!targetClass->isSubclassOf(baseClassName)) {
            return false;
        }

        // Check type parameter compatibility for generic downcast
        if (!targetTypeParams.empty() && !classTypeParams.empty()) {
            // Type parameters must match for valid generic downcast
            return (classTypeParams == targetTypeParams);
        }
        // No type param mismatch, allow downcast
        return true;
    }

    bool TypeExecutor::checkInterfaceMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                           const std::string& targetTypeName) {
        const auto& interfaces = classDef->getImplementedInterfaces();
        for (const auto& iface : interfaces) {
            std::unordered_set<std::string> visited;
            if (checkInterfaceHierarchy(iface, targetTypeName, visited)) {
                return true;
            }
        }
        return false;
    }

    void TypeExecutor::throwIncompatibleCastError(const std::string& className, const std::string& targetTypeName) {
        throwCastError("Cannot cast " + className + " to " + targetTypeName +
                      ": incompatible types in inheritance hierarchy");
    }

    void TypeExecutor::throwCastError(const std::string& message) {
        // Get source location
        auto* loc = context.program->getSourceLocation(context.instructionPointer);
        errors::SourceLocation errorLoc = loc ?
            errors::SourceLocation(loc->filename, loc->line, loc->column) :
            errors::SourceLocation();

        // Throw as UserException with type "Exception" so it can be caught by catch(Exception e)
        // In the future, we could create actual CastError exception objects that extend Exception
        throw errors::UserException(message, "Exception", errorLoc);
    }

    value::Value TypeExecutor::castToObject(const value::Value& val, const std::string& targetTypeName) {
        // Handle null values
        if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
            return val; // null remains null for object casts
        }

        // Handle ValueObject (value classes)
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (!obj) {
                throwCastError("Cannot cast null to " + targetTypeName);
            }
            auto classDef = obj->getClassDefinition();
            std::string className = classDef->getName();

            TypeComponents classComp = extractTypeComponents(className);
            TypeComponents targetComp = extractTypeComponents(targetTypeName);

            bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                           checkInterfaceMatch(classDef, targetTypeName);

            if (canCast) {
                return val;
            } else {
                throwIncompatibleCastError(className, targetTypeName);
                return val;
            }
        }

        // Handle non-object types
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            throwCastError("Cannot cast primitive type to " + targetTypeName);
        }

        // Object cast - check if it's a valid object type
        auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
        if (!obj) {
            throwCastError("Cannot cast null to " + targetTypeName);
        }

        auto classDef = obj->getClassDefinition();
        std::string className = classDef->getName();

        // Extract base class name and type parameters
        TypeComponents classComp = extractTypeComponents(className);
        TypeComponents targetComp = extractTypeComponents(targetTypeName);

        // Check if the object's class can be cast to target type
        bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                       checkUpcastMatch(classDef, targetTypeName, targetComp, classComp.baseName, classComp.typeParams) ||
                       checkDowncastMatch(classComp.baseName, targetComp.baseName, classComp.typeParams, targetComp.typeParams) ||
                       checkInterfaceMatch(classDef, targetTypeName);

        if (canCast) {
            return obj;
        } else {
            throwIncompatibleCastError(className, targetTypeName);
            return val; // Never reached, but satisfies return type
        }
    }

    bool TypeExecutor::checkInterfaceHierarchy(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited
    ) {
        // Avoid infinite loops with circular dependencies
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        // Strip generic parameters for comparison
        auto stripGenerics = [](const std::string& name) -> std::string {
            return ::types::TypeConversionUtils::extractBaseTypeName(name);
        };

        std::string interfaceBaseName = stripGenerics(interfaceName);
        std::string targetBaseName = stripGenerics(targetInterface);

        // Extract type parameters
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

        // Direct match (exact or base without type params)
        if (interfaceName == targetInterface ||
            (interfaceBaseName == targetBaseName && targetTypeParams.empty())) {
            return true;
        }
        // Type param match
        if (interfaceBaseName == targetBaseName && !targetTypeParams.empty()) {
            return (interfaceTypeParams == targetTypeParams);
        }

        // Check extended interfaces recursively
        auto interfaceDef = context.environment->findInterface(interfaceName);
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                if (checkInterfaceHierarchy(extendedInterface, targetInterface, visited)) {
                    return true;
                }
            }
        }

        return false;
    }

    std::string TypeExecutor::valueToString(const value::Value& val) {
        if (std::holds_alternative<int64_t>(val)) {
            return std::to_string(std::get<int64_t>(val));
        }
        if (std::holds_alternative<double>(val)) {
            std::ostringstream oss;
            oss << std::get<double>(val);
            return oss.str();
        }
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
        if (std::holds_alternative<std::string>(val)) {
            return std::get<std::string>(val);
        }
        if (std::holds_alternative<value::InternedString>(val)) {
            return std::get<value::InternedString>(val).getString();
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return "null";
        }
        if (std::holds_alternative<std::monostate>(val)) {
            return "null";
        }
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj && obj->hasField("value") && obj->getFieldCount() == 1) {
                return valueToString(obj->getFieldValue("value"));
            }
            return obj ? "<" + obj->getClassName() + ">" : "null";
        }
        return "<object>";
    }
}
