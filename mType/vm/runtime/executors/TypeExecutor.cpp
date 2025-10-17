#include "TypeExecutor.hpp"
#include "../../../value/StringPool.hpp"
#include <sstream>

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
            return std::holds_alternative<int>(val);
        }
        if (targetTypeName == "Float" || targetTypeName == "float") {
            return std::holds_alternative<float>(val);
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
                std::string baseClassName = utils::TypeConverter::extractBaseTypeName(className);
                std::string baseTargetName = utils::TypeConverter::extractBaseTypeName(targetTypeName);

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
                if (!result) {
                    const auto& interfaces = classDef->getImplementedInterfaces();
                    for (const auto& iface : interfaces) {
                        // Extract base interface name for comparison
                        std::string baseIfaceName = utils::TypeConverter::extractBaseTypeName(iface);

                        if (iface == targetTypeName || baseIfaceName == baseTargetName) {
                            result = true;
                            break;
                        }
                    }
                }

                return result;
            } else {
                return false; // null is not instance of any type
            }
        }
        else if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
            return false; // null is not instance of any type
        }
        else {
            return false; // primitive values are not instances of object types
        }
    }

    value::Value TypeExecutor::castToInt(const value::Value& val) {
        if (std::holds_alternative<int>(val)) {
            return val; // Already int
        }
        else if (std::holds_alternative<float>(val)) {
            return static_cast<int>(std::get<float>(val));
        }
        else if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? 1 : 0;
        }
        else if (std::holds_alternative<std::string>(val)) {
            try {
                return std::stoi(std::get<std::string>(val));
            } catch (...) {
                auto* loc = context.program->getSourceLocation(context.instructionPointer);
                if (loc) {
                    errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                    throw errors::RuntimeException("Cannot cast string to int: " + std::get<std::string>(val), errorLoc);
                } else {
                    throw errors::RuntimeException("Cannot cast string to int: " + std::get<std::string>(val));
                }
            }
        }
        else {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Cannot cast to int from this type", errorLoc);
            } else {
                throw errors::RuntimeException("Cannot cast to int from this type");
            }
        }
    }

    value::Value TypeExecutor::castToFloat(const value::Value& val) {
        if (std::holds_alternative<float>(val)) {
            return val; // Already float
        }
        else if (std::holds_alternative<int>(val)) {
            return static_cast<float>(std::get<int>(val));
        }
        else if (std::holds_alternative<std::string>(val)) {
            try {
                return std::stof(std::get<std::string>(val));
            } catch (...) {
                auto* loc = context.program->getSourceLocation(context.instructionPointer);
                if (loc) {
                    errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                    throw errors::RuntimeException("Cannot cast string to float: " + std::get<std::string>(val), errorLoc);
                } else {
                    throw errors::RuntimeException("Cannot cast string to float: " + std::get<std::string>(val));
                }
            }
        }
        else {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Cannot cast to float from this type", errorLoc);
            } else {
                throw errors::RuntimeException("Cannot cast to float from this type");
            }
        }
    }

    value::Value TypeExecutor::castToString(const value::Value& val) {
        return valueToString(val);
    }

    value::Value TypeExecutor::castToBool(const value::Value& val) {
        if (std::holds_alternative<bool>(val)) {
            return val; // Already bool
        }
        else if (std::holds_alternative<int>(val)) {
            return std::get<int>(val) != 0;
        }
        else if (std::holds_alternative<float>(val)) {
            return std::get<float>(val) != 0.0f;
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
        else {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Cannot cast to bool from this type", errorLoc);
            } else {
                throw errors::RuntimeException("Cannot cast to bool from this type");
            }
        }
    }

    value::Value TypeExecutor::castToObject(const value::Value& val, const std::string& targetTypeName) {
        // Object cast - check if it's a valid object type
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);

            if (obj) {
                auto classDef = obj->getClassDefinition();
                std::string className = classDef->getName();

                // Extract base class name and type parameters from object's class
                std::string baseClassName = className;
                std::string classTypeParams = "";
                size_t genericStart = className.find('<');
                if (genericStart != std::string::npos) {
                    baseClassName = className.substr(0, genericStart);
                    classTypeParams = className.substr(genericStart); // includes <>
                }

                // Extract base target name and type parameters
                std::string baseTargetName = targetTypeName;
                std::string targetTypeParams = "";
                genericStart = targetTypeName.find('<');
                if (genericStart != std::string::npos) {
                    baseTargetName = targetTypeName.substr(0, genericStart);
                    targetTypeParams = targetTypeName.substr(genericStart); // includes <>
                }

                // Check if the object's class can be cast to target type
                bool canCast = false;

                // 1. Exact match (including generic type parameters)
                if (className == targetTypeName) {
                    canCast = true;
                }
                // 2. Base class match without type parameters (e.g., Box<Int> -> Box)
                else if (baseClassName == baseTargetName && targetTypeParams.empty()) {
                    canCast = true;
                }
                // 3. Upcast - object is subclass of target type
                else if (classDef->isSubclassOf(targetTypeName)) {
                    canCast = true;
                }
                // 4. Upcast with base names (for generic types)
                else if (classDef->isSubclassOf(baseTargetName)) {
                    // Check if type parameters match (if target has type params)
                    if (!targetTypeParams.empty() && !classTypeParams.empty()) {
                        // Type parameters must match for generic upcast
                        canCast = (classTypeParams == targetTypeParams);
                    } else {
                        // No type params on target, allow upcast to generic base
                        canCast = true;
                    }
                }
                // 5. Downcast - target type is subclass of object's type (runtime check needed)
                else {
                    // Get target class from registry to check if it's a subclass
                    auto targetClass = context.environment->findClass(baseTargetName);
                    if (targetClass) {
                        // Check if target class is a subclass of the object's actual class
                        if (targetClass->isSubclassOf(baseClassName)) {
                            // Check type parameter compatibility for generic downcast
                            if (!targetTypeParams.empty() && !classTypeParams.empty()) {
                                // Type parameters must match for valid generic downcast
                                canCast = (classTypeParams == targetTypeParams);
                            } else {
                                // No type param mismatch, allow downcast
                                canCast = true;
                            }
                        }
                    }
                }

                // 6. Interface check (with recursive hierarchy checking)
                if (!canCast) {
                    const auto& interfaces = classDef->getImplementedInterfaces();
                    for (const auto& iface : interfaces) {
                        std::unordered_set<std::string> visited;
                        if (checkInterfaceHierarchy(iface, targetTypeName, visited)) {
                            canCast = true;
                            break;
                        }
                    }
                }

                if (canCast) {
                    return obj;
                } else {
                    auto* loc = context.program->getSourceLocation(context.instructionPointer);
                    if (loc) {
                        errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                        throw errors::RuntimeException(
                            "Cannot cast " + className + " to " + targetTypeName +
                            ": incompatible types in inheritance hierarchy", errorLoc);
                    } else {
                        throw errors::RuntimeException(
                            "Cannot cast " + className + " to " + targetTypeName +
                            ": incompatible types in inheritance hierarchy");
                    }
                }
            } else {
                auto* loc = context.program->getSourceLocation(context.instructionPointer);
                if (loc) {
                    errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                    throw errors::RuntimeException("Cannot cast null to " + targetTypeName, errorLoc);
                } else {
                    throw errors::RuntimeException("Cannot cast null to " + targetTypeName);
                }
            }
        }
        else if (std::holds_alternative<std::monostate>(val) || std::holds_alternative<nullptr_t>(val)) {
            return val; // null remains null for object casts
        }
        else {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException("Cannot cast primitive type to " + targetTypeName, errorLoc);
            } else {
                throw errors::RuntimeException("Cannot cast primitive type to " + targetTypeName);
            }
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
            return utils::TypeConverter::extractBaseTypeName(name);
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
        if (std::holds_alternative<int>(val)) {
            return std::to_string(std::get<int>(val));
        }
        if (std::holds_alternative<float>(val)) {
            std::ostringstream oss;
            oss << std::get<float>(val);
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
        return "<object>";
    }
}
