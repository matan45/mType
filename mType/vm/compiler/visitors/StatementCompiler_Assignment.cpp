#include "StatementCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"

namespace vm::compiler::visitors
{
    value::Value StatementCompiler::compileAssignment(ast::AssignmentNode* node)
    {
        std::string name = node->getVariableName();
        value::ValueType varType = node->getVariableType();
        auto* value = node->getValue();

        std::string existingClassName;
        bool isReassignment = detectReassignment(name, existingClassName);

        validateClassOrInterfaceType(node);
        validateLambdaAssignment(node, isReassignment, existingClassName);

        // PHASE 3: reassignment type validation runs AFTER value compilation so the
        // generic-function return-type cache is populated.

        if (value)
        {
            bool pushedContext = false;
            bool autoBoxed = false;
            bool needsAutoUnbox = false;

            std::string targetClassName = isReassignment ? existingClassName : ctx.resolveGenericType(node->getClassName());

            bool shouldAutoBox = false;
            if (!targetClassName.empty())
            {
                if (varType == value::ValueType::OBJECT)
                {
                    shouldAutoBox = true;
                }
                else if (isReassignment)
                {
                    shouldAutoBox = (targetClassName == "Int" || targetClassName == "Float" ||
                                    targetClassName == "Bool" || targetClassName == "String");
                }
            }

            if (shouldAutoBox)
            {
                autoBoxed = tryEmitAutoBoxing(value, targetClassName);
            }

            if (!autoBoxed)
            {
                if (varType != value::ValueType::VOID)
                {
                    std::string expectedClassName = isReassignment ? existingClassName : ctx.resolveGenericType(node->getClassName());

                    types::ExpectedTypeContext expectedCtx(varType, expectedClassName);
                    ctx.pushExpectedTypeContext(expectedCtx);
                    pushedContext = true;
                }

                value->accept(ctx.visitor);

                if (pushedContext)
                {
                    ctx.popExpectedTypeContext();
                }
            }

            if (isReassignment)
            {
                if (!existingClassName.empty())
                {
                    std::string valueClassName = ctx.typeInference.inferExpressionClassName(value);
                    value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
                    bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(value);

                    bool existingIsNullable = true;
                    std::string varName = node->getVariableName();
                    if (ctx.functionFrameManager.isInFunction())
                    {
                        existingIsNullable = ctx.variableTracker.getLocalNullableByName(varName);
                    }
                    else if (ctx.globalRegistry.exists(varName))
                    {
                        existingIsNullable = ctx.globalRegistry.isNullable(varName);
                    }

                    if (!valueClassName.empty() || valueType != value::ValueType::OBJECT)
                    {
                        std::string resolvedExistingClassName = ctx.resolveGenericType(existingClassName);
                        std::string resolvedValueClassName = ctx.resolveGenericType(valueClassName);

                        ctx.typeValidator.validateAssignment(value::ValueType::OBJECT, resolvedExistingClassName,
                                                             valueType, resolvedValueClassName, isNullValue, node->getLocation(),
                                                             existingIsNullable);
                    }
                }
            }
            else if (varType != value::ValueType::VOID)
            {
                value::ValueType valueType = ctx.typeInference.inferExpressionType(value);
                std::string varClassName = ctx.resolveGenericType(node->getClassName());
                std::string valueClassName = ctx.resolveGenericType(ctx.typeInference.inferExpressionClassName(value));
                bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(value);

                bool canAutoBox = autoBoxed;
                if (!canAutoBox && varType == value::ValueType::OBJECT &&
                    ((varClassName == "Int" && valueType == value::ValueType::INT) ||
                     (varClassName == "Float" && valueType == value::ValueType::FLOAT) ||
                     (varClassName == "Bool" && valueType == value::ValueType::BOOL) ||
                     (varClassName == "String" && valueType == value::ValueType::STRING)))
                {
                    canAutoBox = true;
                }

                if (!canAutoBox)
                {
                    // MYT-137: target-type-guided array-literal inference. The RHS
                    // literal `[new A(), new B(), new C()]` is otherwise typed as
                    // `A[]` (first-element rule in TypeInferenceEngine) which forces
                    // the strict invariance check below to reject `Shape[] = A[]`.
                    // Validate each NewNode element against the declared element
                    // type and, on success, retype the literal as the declared
                    // array type so the strict check passes for valid covariant
                    // literals while still rejecting `Animal[] = new Dog[]` (which
                    // is a NewArrayCreation, not an ArrayLiteral).
                    auto* arrayLit = dynamic_cast<ast::nodes::expressions::ArrayLiteralNode*>(value);
                    if (arrayLit && varClassName.find("[]") != std::string::npos) {
                        std::string declaredElementType = varClassName.substr(0, varClassName.find("[]"));
                        const auto& elements = arrayLit->getElements();

                        // Apply target-type retype only when every element is a
                        // NewNode we can validate here. If any element is a
                        // variable ref, call, or other expression, fall through
                        // to the standard validator — silently retyping would
                        // skip the type-check for those elements entirely.
                        bool allElementsAreNew = !elements.empty();
                        for (const auto& element : elements) {
                            if (!dynamic_cast<ast::NewNode*>(element.get())) {
                                allElementsAreNew = false;
                                break;
                            }
                        }

                        if (allElementsAreNew) {
                            for (size_t i = 0; i < elements.size(); ++i) {
                                auto* newNode = dynamic_cast<ast::NewNode*>(elements[i].get());
                                std::string elementClass = newNode->getClassName();
                                if (elementClass != declaredElementType &&
                                    !ctx.typeValidator.isClassCompatible(elementClass, declaredElementType)) {
                                    throw errors::TypeException(
                                        "Array literal type mismatch: expected '" + declaredElementType +
                                        "' but got '" + elementClass + "' at index " + std::to_string(i),
                                        node->getLocation());
                                }
                            }
                            valueClassName = varClassName;
                        }
                    }

                    ctx.typeValidator.validateAssignment(varType, varClassName, valueType,
                                                         valueClassName, isNullValue, node->getLocation(),
                                                         node->isNullableType());
                }

                // Auto-unbox: Box-typed value into a primitive-typed variable.
                if (!autoBoxed && varType != value::ValueType::OBJECT)
                {
                    if (valueType == value::ValueType::OBJECT)
                    {
                        if ((varType == value::ValueType::INT && valueClassName == "Int") ||
                            (varType == value::ValueType::FLOAT && valueClassName == "Float") ||
                            (varType == value::ValueType::BOOL && valueClassName == "Bool") ||
                            (varType == value::ValueType::STRING && valueClassName == "String"))
                        {
                            needsAutoUnbox = true;
                            size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                            ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                         static_cast<uint64_t>(methodNameIndex),
                                                         0u,
                                                         value);
                        }
                    }
                }
            }
        }
        else
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        }

        if (varType != value::ValueType::VOID)
        {
            emitVariableDeclaration(node);
        }
        else
        {
            emitVariableReassignment(node, isReassignment);
        }

        return std::monostate{};
    }
}
