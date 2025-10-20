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
#include "../../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../validation/AccessValidator.hpp"
#include "../base/AccessContext.hpp"
#include <string>
#include <vector>

using namespace errors;
using namespace runtimeTypes::klass;

namespace evaluator
{
    namespace expressions
    {
        AccessHandler::AccessHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr)
        {
        }

        void AccessHandler::setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        void AccessHandler::setObjectEvaluator(interfaces::IObjectEvaluator* evaluator)
        {
            objEvaluator = evaluator;
        }

        void AccessHandler::setStatementEvaluator(interfaces::IStatementEvaluator* evaluator)
        {
            stmtEvaluator = evaluator;
        }

        Value AccessHandler::evaluateVariableAccess(VariableNode* node)
        {
            // Handle 'this' keyword specifically
            if (node->getName() == "this")
            {
                return handleThisKeyword(node);
            }

            std::string varName = node->getName();
            auto env = context->getEnvironment();

            // Check if this is a qualified static field access (contains ::)
            if (varName.find("::") != std::string::npos)
            {
                return handleQualifiedStaticAccess(varName, node);
            }

            // Priority order:
            // 1. Parameters and local variables (in any scope) - always take priority
            // 2. Instance fields (implicit this.field) - for bare field names in instance methods
            // 3. Global variables - last resort
            // 4. Static fields

            // Check if variable exists in any scope (parameters, local vars, globals)
            auto scopeManager = env->getScopeManager();
            auto varDef = env->findVariable(varName);

            // If found in a non-global scope (parameter or local variable), use it immediately
            // Parameters and local variables should always shadow fields
            if (varDef)
            {
                // Check if this variable is in the global scope
                auto globalScope = scopeManager->getGlobalScope();
                auto globalVar = globalScope->findVariable(varName);

                // If variable is NOT in global scope, it's a parameter or local variable
                // Parameters and local variables take absolute priority
                if (!globalVar || globalVar != varDef)
                {
                    return varDef->getValue();
                }
                // If it IS in global scope, we continue to check instance fields first
            }

            // Check instance fields (implicit this.field) before using global variables
            // This allows instance fields to shadow global variables
            auto currentInstance = context->getCurrentInstance();
            if (currentInstance)
            {
                Value result = handleInstanceFieldAccess(varName, node);
                if (!std::holds_alternative<std::monostate>(result))
                {
                    return result;
                }
            }

            // Use global variable if we found one earlier
            if (varDef)
            {
                return varDef->getValue();
            }

            // Check static fields
            Value staticResult = handleStaticFieldAccess(varName, node);
            if (!std::holds_alternative<std::monostate>(staticResult))
            {
                return staticResult;
            }

            throw UndefinedException("Undefined variable: " + varName, node->getLocation());
        }

        Value AccessHandler::handleThisKeyword(VariableNode* node)
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

        std::vector<std::string> AccessHandler::parseQualifiedName(const std::string& qualifiedName)
        {
            std::vector<std::string> parts;
            size_t start = 0;
            size_t pos = qualifiedName.find("::");

            while (pos != std::string::npos)
            {
                parts.push_back(qualifiedName.substr(start, pos - start));
                start = pos + 2; // Skip "::"
                pos = qualifiedName.find("::", start);
            }
            parts.push_back(qualifiedName.substr(start));

            return parts;
        }

        Value AccessHandler::handleQualifiedStaticAccess(const std::string& varName, VariableNode* node)
        {
            auto parts = parseQualifiedName(varName);

            // Handle qualified static field access: ClassName::fieldName
            if (parts.size() == 2)
            {
                std::string className = parts[0];
                std::string fieldName = parts[1];

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

        Value AccessHandler::handleInstanceFieldAccess(const std::string& varName, VariableNode* node)
        {
            auto currentInstance = context->getCurrentInstance();
            if (!currentInstance)
            {
                return std::monostate{}; // Not found
            }

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

            return std::monostate{}; // Not found
        }

        Value AccessHandler::handleStaticFieldAccess(const std::string& varName, VariableNode* node)
        {
            auto env = context->getEnvironment();
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
                            // ACCESS CONTROL: Validate field access permissions
                            auto callingClassName = context->getCurrentCallingClass();
                            auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
                            auto accessContext = base::AccessContext::forStaticAccess(
                                callingClassName,
                                classDef,
                                node->getLocation(),
                                callingClassDef
                            );
                            validation::AccessValidator::validateFieldAccess(accessContext, *field);

                            // Use ObjectEvaluator to properly access static member through InstanceManager
                            if (objEvaluator)
                            {
                                return objEvaluator->accessStaticMember(className, varName);
                            }
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
                        // ACCESS CONTROL: Validate field access permissions
                        auto callingClassName = context->getCurrentCallingClass();
                        auto callingClassDef = callingClassName.empty() ? nullptr : env->findClass(callingClassName);
                        auto accessContext = base::AccessContext::forStaticAccess(
                            callingClassName,
                            classDef,
                            node->getLocation(),
                            callingClassDef
                        );
                        validation::AccessValidator::validateFieldAccess(accessContext, *field);

                        // Use ObjectEvaluator to properly access static member through InstanceManager
                        if (objEvaluator)
                        {
                            return objEvaluator->accessStaticMember(className, varName);
                        }
                        return field->getValue();
                    }
                }
            }

            return std::monostate{}; // Not found
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
            if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
            {
                auto object = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
                auto field = object->getField(node->getMemberName());

                if (!field)
                {
                    throw UndefinedException("Undefined field: " + node->getMemberName(), node->getLocation());
                }

                // ACCESS CONTROL: Validate field access permissions
                auto classDef = object->getClassDefinition();
                auto callingInstance = context->getCurrentInstance();
                auto accessContext = base::AccessContext::forInstanceAccess(
                    callingInstance,
                    classDef,
                    node->getLocation()
                );
                validation::AccessValidator::validateFieldAccess(accessContext, *field);

                return object->getFieldValue(node->getMemberName());
            }

            // Handle native arrays
            if (std::holds_alternative<std::shared_ptr<NativeArray>>(objectValue))
            {
                auto array = std::get<std::shared_ptr<NativeArray>>(objectValue);
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
            if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(objectValue))
            {
                auto flatArray = std::get<std::shared_ptr<FlatMultiArray>>(objectValue);
                if (node->getMemberName() == "length")
                {
                    // For multi-dimensional arrays, return the first dimension size (like in Java/C#)
                    return static_cast<int>(flatArray->getDimensions()[0]);
                }
                else
                {
                    throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                             node->getLocation());
                }
            }

            // Handle SparseMultiArray (adaptive sparse arrays)
            if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(objectValue))
            {
                auto sparseArray = std::get<std::shared_ptr<SparseMultiArray>>(objectValue);
                if (node->getMemberName() == "length")
                {
                    // For sparse multi-dimensional arrays, return the first dimension size
                    return static_cast<int>(sparseArray->getDimensions()[0]);
                }
                else
                {
                    throw UndefinedException("Array does not have member '" + node->getMemberName() + "'",
                                             node->getLocation());
                }
            }

            // Handle FlatMultiObjectArray (Structure-of-Arrays for objects)
            if (std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(objectValue))
            {
                auto flatMultiObjectArray = std::get<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(objectValue);
                if (node->getMemberName() == "length")
                {
                    // For multi-dimensional object arrays, return the first dimension size
                    return static_cast<int>(flatMultiObjectArray->getDimensions()[0]);
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

            throw UndefinedException("Object evaluator not available for object creation", node->getLocation());
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
    }
}
