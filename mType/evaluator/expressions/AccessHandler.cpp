#include "AccessHandler.hpp"
#include "../ExpressionEvaluator.hpp"
#include "../ObjectEvaluator.hpp"
#include "../StatementEvaluator.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include <string>
#include <vector>

using namespace errors;
using namespace runtimeTypes::klass;

namespace evaluator {
namespace expressions {

    Value AccessHandler::evaluateVariableAccess(VariableNode* node)
    {
        // Handle 'this' keyword specifically
        if (node->getName() == "this")
        {
            // VALIDATION: Prevent 'this' access from static methods
            if (context->isInStaticMethodContext())
            {
                throw TypeException("Cannot use 'this' in static method context", node->getLocation());
            }

            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                return currentInstance;
            }
            throw UndefinedException("'this' is not available outside of instance methods", node->getLocation());
        }

        std::string varName = node->getName();

        // Check if this is a qualified static field access (contains ::)
        if (varName.find("::") != std::string::npos)
        {
            // Parse the qualified name into parts
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = varName.find("::");

            while (pos != std::string::npos)
            {
                parts.push_back(varName.substr(start, pos - start));
                start = pos + 2; // Skip "::"
                pos = varName.find("::", start);
            }
            parts.push_back(varName.substr(start));

            // Handle qualified static field access: ClassName::fieldName
            if (parts.size() == 2)
            {
                std::string className = parts[0];
                std::string fieldName = parts[1];
                // Delegate static member access to object evaluator
                if (objEvaluator)
                {
                    return objEvaluator->accessStaticMember(className, fieldName);
                }
                else
                {
                    throw UndefinedException("Object evaluator not available for static member access",
                                             node->getLocation());
                }
            }
            else
            {
                throw UndefinedException("Complex qualified variable access not supported: '" + varName + "'",
                                         node->getLocation());
            }
        }

        auto env = context->getEnvironment();

        // Check variables first (parameters, local variables, etc.)
        auto varDef = env->findVariable(varName);
        if (varDef)
        {
            return varDef->getValue();
        }

        // Check instance fields if no variable found
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance)
        {
            // VALIDATION: Prevent instance member access from static methods
            if (context->isInStaticMethodContext())
            {
                auto field = currentInstance->getField(varName);
                if (field && !field->isStatic())
                {
                    throw TypeException("Cannot access instance field '" + varName +
                                        "' from static method context", node->getLocation());
                }
            }

            auto field = currentInstance->getField(varName);
            if (field)
            {
                return currentInstance->getFieldValue(varName);
            }
        }

        // Check if this might be a static field access
        // First check if we're in a static method by looking for the current class name
        auto classRegistry = env->getClassRegistry();

        // Check if we have a current class name stored (from static method execution)
        auto currentClassVar = env->findVariable("__current_class_name__");
        if (currentClassVar)
        {
            auto currentClassValue = currentClassVar->getValue();
            if (std::holds_alternative<std::string>(currentClassValue))
            {
                std::string className = std::get<std::string>(currentClassValue);
                auto classDef = env->findClass(className);
                if (classDef)
                {
                    auto field = classDef->getField(varName);
                    if (field && field->isStatic())
                    {
                        return field->getValue();
                    }
                }
            }
        }

        // Fallback: search all classes (for backward compatibility)
        auto allClassNames = classRegistry->getAllItemNames();
        std::string currentClassName;
        if (currentClassVar)
        {
            auto currentClassValue = currentClassVar->getValue();
            if (std::holds_alternative<std::string>(currentClassValue))
            {
                currentClassName = std::get<std::string>(currentClassValue);
            }
        }

        for (const auto& className : allClassNames)
        {
            // Skip if we already checked this class above
            if (!currentClassName.empty() && className == currentClassName) continue;

            auto classDef = classRegistry->findClass(className);
            if (classDef)
            {
                auto field = classDef->getField(varName);
                if (field && field->isStatic())
                {
                    return field->getValue();
                }
            }
        }

        throw UndefinedException("Undefined variable: " + varName, node->getLocation());
    }

    Value AccessHandler::evaluateMemberAccess(MemberAccessNode* node)
    {
        Value objectValue = exprEvaluator->evaluate(node->getObject());

        // Check if object is null
        if (std::holds_alternative<nullptr_t>(objectValue))
        {
            throw TypeException("Cannot access member of null object", node->getLocation());
        }

        // Handle object instances
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            auto object = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
            auto field = object->getField(node->getMemberName());

            if (!field)
            {
                throw UndefinedException("Undefined field: " + node->getMemberName(), node->getLocation());
            }

            return object->getFieldValue(node->getMemberName());
        }

        // Handle native arrays
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                return static_cast<int>(array->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        // Handle FlatMultiArray (multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(objectValue))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                // For multi-dimensional arrays, return the first dimension size (like in Java/C#)
                return static_cast<int>(flatArray->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        // Handle SparseMultiArray (adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(objectValue))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                // For sparse multi-dimensional arrays, return the first dimension size
                return static_cast<int>(sparseArray->size());
            }
            else
            {
                throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                         node->getLocation());
            }
        }

        throw TypeException("Cannot access member of non-object value", node->getLocation());
    }

    Value AccessHandler::evaluateObjectCreation(NewNode* node)
    {
        // Delegate to ObjectEvaluator
        if (objEvaluator)
        {
            return objEvaluator->evaluate(node);
        }
        else
        {
            throw UndefinedException("Object evaluator not available for object creation", node->getLocation());
        }
    }

    Value AccessHandler::evaluateAssignment(AssignmentNode* node)
    {
        // Delegate assignment to the statement evaluator since it handles the actual assignment logic
        if (!stmtEvaluator)
        {
            throw TypeException("Statement evaluator not available for assignment expression");
        }

        // The statement evaluator's evaluateAssignmentNode already returns the assigned value
        return stmtEvaluator->evaluate(node);
    }

} // namespace expressions
} // namespace evaluator
