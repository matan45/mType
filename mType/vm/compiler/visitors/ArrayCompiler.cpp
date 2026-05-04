#include "ArrayCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../errors/TypeException.hpp"

namespace vm::compiler::visitors
{
    ArrayCompiler::ArrayCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value ArrayCompiler::compileArrayCreation(ast::ArrayCreationNode* node)
    {
        const auto& sizeExpressions = node->getSizeExpressions();
        size_t dimensionCount = node->getDimensionCount();

        if (dimensionCount == 1) {
            // Single-dimensional array: new Type[size]
            // Compile size expression and push onto stack
            sizeExpressions[0]->accept(ctx.visitor);  // Will need visitor delegation

            // Get element type info
            const auto& typeInfo = node->getElementTypeInfo();
            size_t typeNameIndex = ctx.program.getConstantPool().addString(typeInfo.toString());

            // Emit NEW_ARRAY with element type and source location
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_ARRAY, static_cast<uint64_t>(typeNameIndex), node);
        }
        else {
            // Multi-dimensional array: new Type[size1][size2]...
            size_t specifiedDimensions = 0;
            for (const auto& sizeExpr : sizeExpressions) {
                if (sizeExpr) {
                    sizeExpr->accept(ctx.visitor);  // Will need visitor delegation
                    specifiedDimensions++;
                }
            }

            // Get element type info
            const auto& typeInfo = node->getElementTypeInfo();
            size_t typeNameIndex = ctx.program.getConstantPool().addString(typeInfo.toString());

            // Emit NEW_ARRAY_MULTI with element type, total dimensions, and specified dimensions
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_ARRAY_MULTI,
                           std::vector<uint64_t>{
                               static_cast<uint64_t>(typeNameIndex),
                               static_cast<uint64_t>(dimensionCount),
                               static_cast<uint64_t>(specifiedDimensions)
                           }, node);
        }

        return std::monostate{};
    }

    value::Value ArrayCompiler::compileArrayLiteral(ast::ArrayLiteralNode* node)
    {
        const auto& elements = node->getElements();
        size_t elementCount = elements.size();

        // Type validation - check that all elements have compatible types
        if (elementCount > 0) {
            std::string expectedType;
            bool isFirstElement = true;

            for (size_t i = 0; i < elementCount; ++i) {
                std::string currentType;

                // Determine type of current element
                if (dynamic_cast<ast::IntegerNode*>(elements[i].get())) {
                    currentType = "int";
                } else if (dynamic_cast<ast::FloatNode*>(elements[i].get())) {
                    currentType = "float";
                } else if (dynamic_cast<ast::StringNode*>(elements[i].get())) {
                    currentType = "string";
                } else if (dynamic_cast<ast::BoolNode*>(elements[i].get())) {
                    currentType = "bool";
                } else if (dynamic_cast<ast::NullNode*>(elements[i].get())) {
                    currentType = "null";
                } else if (auto* newNode = dynamic_cast<ast::NewNode*>(elements[i].get())) {
                    currentType = "object:" + newNode->getClassName();
                } else {
                    currentType = "unknown";
                }

                if (isFirstElement) {
                    expectedType = currentType;
                    isFirstElement = false;
                } else {
                    // Check for null in primitive arrays
                    if (currentType == "null" && (expectedType == "int" || expectedType == "float" ||
                                                   expectedType == "bool" || expectedType == "string")) {
                        throw errors::TypeException(
                            "Array literal cannot contain null in primitive array (expected '" + expectedType + "')",
                            node->getLocation()
                        );
                    }

                    // Type compatibility checks
                    if (currentType != "null" && currentType != "unknown" && expectedType != "unknown") {
                        if (currentType != expectedType) {
                            // Allow int/float mixing
                            if ((currentType == "int" && expectedType == "float") ||
                                (currentType == "float" && expectedType == "int")) {
                                // int/float mixing is allowed
                            }
                            // Allow all object types in array literals - compatibility is
                            // validated at declaration level by TypeValidator
                            else if (currentType.substr(0, 7) == "object:" && expectedType.substr(0, 7) == "object:") {
                                continue;
                            }
                            else {
                                // Type mismatch error
                                throw errors::TypeException(
                                    "Array literal type mismatch: expected '" + expectedType +
                                    "' but got '" + currentType + "' at index " + std::to_string(i),
                                    node->getLocation()
                                );
                            }
                        }
                    }
                }
            }
        }

        // Push array size onto stack
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                       static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(static_cast<int64_t>(elementCount))), node);

        // Create array with generic "Object" type (type will be inferred from elements)
        size_t typeNameIndex = ctx.program.getConstantPool().addString("Object");
        ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_ARRAY, static_cast<uint64_t>(typeNameIndex), node);

        // Array is now on stack. For each element:
        // 1. Duplicate array reference
        // 2. Push index
        // 3. Push element value (with auto-boxing if needed)
        // 4. Call ARRAY_SET
        for (size_t i = 0; i < elementCount; ++i) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::DUP, node);  // Duplicate array reference

            // Push index
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT,
                           static_cast<uint64_t>(ctx.program.getConstantPool().addInteger(static_cast<int64_t>(i))), node);

            // Compile element value
            // Note: Auto-boxing for array literals is handled by VariableCompiler
            // during array variable declaration, not here
            elements[i]->accept(ctx.visitor);

            // Set array element with source location
            ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_SET, node);
        }

        // Array reference is still on stack

        return std::monostate{};
    }

    value::Value ArrayCompiler::compileIndexAccess(ast::IndexAccessNode* node)
    {
        // Compile array/collection expression
        node->getCollection()->accept(ctx.visitor);  // Will need visitor delegation

        // Compile index expression
        node->getIndex()->accept(ctx.visitor);  // Will need visitor delegation

        // Emit ARRAY_GET to retrieve element with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_GET, node);

        return std::monostate{};
    }

    value::Value ArrayCompiler::compileIndexAssignment(ast::IndexAssignmentNode* node)
    {
        // Compile array/collection expression
        node->getObject()->accept(ctx.visitor);  // Will need visitor delegation

        // Compile index expression
        node->getIndex()->accept(ctx.visitor);  // Will need visitor delegation

        // PHASE 4: Check for auto-boxing (primitive value to Box array element)
        bool autoBoxed = false;
        value::ValueType arrayType = ctx.typeInference.inferExpressionType(node->getObject());

        if (arrayType == value::ValueType::ARRAY)
        {
            std::string arrayClassName = ctx.typeInference.inferExpressionClassName(node->getObject());
            // Extract element type from array type (e.g., "Int[]" -> "Int")
            size_t bracketPos = arrayClassName.find('[');
            if (bracketPos != std::string::npos && arrayClassName.back() == ']')
            {
                std::string elementType = arrayClassName.substr(0, bracketPos);

                // Get value type information for validation
                value::ValueType valueType = ctx.typeInference.inferExpressionType(node->getValue());
                std::string valueClassName = ctx.typeInference.inferExpressionClassName(node->getValue());
                bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(node->getValue());

                // Check if element type is a Box type and value needs boxing
                if (elementType == "Int" || elementType == "Float" ||
                    elementType == "Bool" || elementType == "String")
                {
                    if ((elementType == "Int" && valueType == value::ValueType::INT) ||
                        (elementType == "Float" && valueType == value::ValueType::FLOAT) ||
                        (elementType == "Bool" && valueType == value::ValueType::BOOL) ||
                        (elementType == "String" && valueType == value::ValueType::STRING))
                    {
                        // Auto-box: compile value, then wrap in NEW_OBJECT or NEW_VALUE_OBJECT
                        node->getValue()->accept(ctx.visitor);
                        size_t classNameIndex = ctx.program.getConstantPool().addString(elementType);
                        auto boxClassDef = ctx.env->findClass(elementType);
                        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
                        if (boxIsValue) {
                            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                                         static_cast<uint64_t>(classNameIndex),
                                                         1u, node->getValue());
                            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, node->getValue());
                        } else {
                            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                                         static_cast<uint64_t>(classNameIndex),
                                                         1u, node->getValue());
                        }
                        autoBoxed = true;
                    }
                }

                // Type validation: ensure value is compatible with array element type
                // Skip validation if auto-boxing occurred (already validated above)
                if (!autoBoxed)
                {
                    // Only validate for object array elements (not primitives)
                    bool isPrimitiveArray = (elementType == "int" || elementType == "float" ||
                                            elementType == "bool" || elementType == "string");

                    if (!isPrimitiveArray && valueType == value::ValueType::OBJECT)
                    {
                        // Use TypeValidator to check if value type is compatible with element type
                        ctx.typeValidator.validateAssignment(
                            value::ValueType::OBJECT, elementType,  // Expected: array element type
                            valueType, valueClassName,               // Actual: value being assigned
                            isNullValue, node->getLocation()
                        );
                    }
                }
            }
        }

        // If not auto-boxed, compile value normally
        if (!autoBoxed)
        {
            node->getValue()->accept(ctx.visitor);
        }

        // Emit ARRAY_SET to store element with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::ARRAY_SET, node);

        return std::monostate{};
    }
}

