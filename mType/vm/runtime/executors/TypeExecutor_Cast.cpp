#include "TypeExecutor.hpp"
#include <cstdint>
#include "../utils/NullCheckUtils.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../errors/UserException.hpp"

namespace vm::runtime
{
    value::Value TypeExecutor::castToInt(const value::Value& val) {
        if (value::isInt(val)) {
            return val;
        }
        if (value::isFloat(val)) {
            return static_cast<int64_t>(value::asFloat(val));
        }
        if (value::isBool(val)) {
            return value::asBool(val) ? static_cast<int64_t>(1) : static_cast<int64_t>(0);
        }
        if (value::isAnyString(val)) {
            // MYT-317: folds STRING_INLINE alongside heap STRING / INTERNED_STRING.
            std::string str(value::asStringView(val));
            try {
                return static_cast<int64_t>(std::stoll(str));
            } catch (...) {
                throwCastError("Cannot cast string to int: " + str);
            }
        }
        if (value::isAnyObject(val)) {
            // MYT-208: accept STACK_OBJECT (cast on a stack-promoted Int box).
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Int") {
                return val;
            }
            throwCastError("Cannot cast object to int");
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Int") {
                return val;
            }
            throwCastError("Cannot cast value object to int");
        }
        throwCastError("Cannot cast to int from this type");
    }

    value::Value TypeExecutor::castToFloat(const value::Value& val) {
        if (value::isFloat(val)) {
            return val;
        }
        if (value::isInt(val)) {
            return static_cast<double>(value::asInt(val));
        }
        if (value::isAnyString(val)) {
            // MYT-317: SSO-aware string→float cast.
            std::string str(value::asStringView(val));
            try {
                return std::stod(str);
            } catch (...) {
                throwCastError("Cannot cast string to float: " + str);
            }
        }
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Float") {
                return val;
            }
            throwCastError("Cannot cast object to float");
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Float") {
                return val;
            }
            throwCastError("Cannot cast value object to float");
        }
        throwCastError("Cannot cast to float from this type");
    }

    value::Value TypeExecutor::castToString(const value::Value& val) {
        // A String boxed-class instance must pass through unchanged — unboxing
        // to a primitive string here would strip the wrapper and break later
        // method dispatch (e.g. `(String)list.get(i)` then `.getValue()`).
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "String") {
                return val;
            }
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "String") {
                return val;
            }
        }
        return valueToString(val);
    }

    value::Value TypeExecutor::castToBool(const value::Value& val) {
        if (value::isBool(val)) {
            return val;
        }
        if (value::isInt(val)) {
            return value::asInt(val) != 0;
        }
        if (value::isFloat(val)) {
            return value::asFloat(val) != 0.0;
        }
        if (value::isAnyString(val)) {
            // MYT-317: SSO-aware. Non-empty strings of any form are true.
            return !value::asStringView(val).empty();
        }
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Bool") {
                return val;
            }
            throwCastError("Cannot cast object to bool");
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Bool") {
                return val;
            }
            throwCastError("Cannot cast value object to bool");
        }
        throwCastError("Cannot cast to bool from this type");
    }

    value::Value TypeExecutor::castToObject(const value::Value& val, const std::string& targetTypeName) {
        if (utils::isNullValue(val)) {
            return val;
        }

        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (!obj) {
                throwCastError("Cannot cast null to " + targetTypeName);
            }
            auto classDef = obj->getClassDefinition();
            std::string className = classDef->getName();

            TypeComponents classComp = extractTypeComponents(className);
            TypeComponents targetComp = extractTypeComponents(targetTypeName);

            bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                           matchInterfaceWalk(classDef, obj->getGenericTypeBindings(),
                                              targetTypeName, environment.get());

            if (canCast) {
                return val;
            }
            throwIncompatibleCastError(className, targetTypeName);
            return val;
        }

        // MYT-208: accept STACK_OBJECT — `(Point) p` on a stack-promoted local
        // is valid and cheap; raw pointer access is enough for the cast.
        if (!value::isAnyObject(val)) {
            throwCastError("Cannot cast primitive type to " + targetTypeName);
        }

        auto* obj = value::asObjectInstanceRaw(val);
        if (!obj) {
            throwCastError("Cannot cast null to " + targetTypeName);
        }

        auto classDef = obj->getClassDefinition();
        std::string className = classDef->getName();

        TypeComponents classComp = extractTypeComponents(className);
        TypeComponents targetComp = extractTypeComponents(targetTypeName);

        bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                       checkUpcastMatch(classDef, targetTypeName, targetComp, classComp.baseName, classComp.typeParams) ||
                       checkDowncastMatch(classComp.baseName, targetComp.baseName, classComp.typeParams, targetComp.typeParams) ||
                       matchInterfaceWalk(classDef, obj->getGenericTypeBindings(),
                                          targetTypeName, environment.get());

        if (canCast) {
            // Cast is a runtime type check — preserve the original Value's tag
            // (OBJECT or STACK_OBJECT). Returning the raw `obj` pointer here
            // would silently match Value(bool) via pointer→bool decay.
            return val;
        }
        throwIncompatibleCastError(className, targetTypeName);
        return val;
    }

    void TypeExecutor::throwIncompatibleCastError(const std::string& className, const std::string& targetTypeName) {
        throwCastError("Cannot cast " + className + " to " + targetTypeName +
                      ": incompatible types in inheritance hierarchy");
    }

    void TypeExecutor::throwCastError(const std::string& message) {
        auto* loc = context.program->getSourceLocation(context.instructionPointer);
        errors::SourceLocation errorLoc = loc ?
            errors::SourceLocation(loc->filename, loc->line, loc->column) :
            errors::SourceLocation();

        // UserException with type "Exception" so it can be caught by
        // catch(Exception e). Future work could create a CastError exception
        // class that extends Exception.
        throw errors::UserException(message, "Exception", errorLoc);
    }
}
