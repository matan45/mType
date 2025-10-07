#include "ArrayCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"

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

            // Emit NEW_ARRAY with element type
            ctx.program.emit(bytecode::OpCode::NEW_ARRAY, static_cast<uint32_t>(typeNameIndex));
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
            ctx.program.emit(bytecode::OpCode::NEW_ARRAY_MULTI,
                           std::vector<uint32_t>{
                               static_cast<uint32_t>(typeNameIndex),
                               static_cast<uint32_t>(dimensionCount),
                               static_cast<uint32_t>(specifiedDimensions)
                           });
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
                    // Type compatibility checks
                    if (currentType != "null" && currentType != "unknown" && expectedType != "unknown") {
                        if (currentType != expectedType) {
                            // Allow int/float mixing
                            if (!((currentType == "int" && expectedType == "float") ||
                                  (currentType == "float" && expectedType == "int"))) {
                                // Type mismatch - but we'll allow it for now
                                // The VM will handle runtime type checking
                            }
                        }
                    }
                }
            }
        }

        // Push array size onto stack
        ctx.program.emit(bytecode::OpCode::PUSH_INT,
                       static_cast<uint32_t>(ctx.program.getConstantPool().addInteger(static_cast<int>(elementCount))));

        // Create array with generic "Object" type (type will be inferred from elements)
        size_t typeNameIndex = ctx.program.getConstantPool().addString("Object");
        ctx.program.emit(bytecode::OpCode::NEW_ARRAY, static_cast<uint32_t>(typeNameIndex));

        // Array is now on stack. For each element:
        // 1. Duplicate array reference
        // 2. Push index
        // 3. Push element value
        // 4. Call ARRAY_SET
        for (size_t i = 0; i < elementCount; ++i) {
            ctx.program.emit(bytecode::OpCode::DUP);  // Duplicate array reference

            // Push index
            ctx.program.emit(bytecode::OpCode::PUSH_INT,
                           static_cast<uint32_t>(ctx.program.getConstantPool().addInteger(static_cast<int>(i))));

            // Compile and push element value
            elements[i]->accept(ctx.visitor);

            // Set array element
            ctx.program.emit(bytecode::OpCode::ARRAY_SET);
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

        // Emit ARRAY_GET to retrieve element
        ctx.program.emit(bytecode::OpCode::ARRAY_GET);

        return std::monostate{};
    }

    value::Value ArrayCompiler::compileIndexAssignment(ast::IndexAssignmentNode* node)
    {
        // Compile array/collection expression
        node->getObject()->accept(ctx.visitor);  // Will need visitor delegation

        // Compile index expression
        node->getIndex()->accept(ctx.visitor);  // Will need visitor delegation

        // Compile value expression
        node->getValue()->accept(ctx.visitor);  // Will need visitor delegation

        // Emit ARRAY_SET to store element
        ctx.program.emit(bytecode::OpCode::ARRAY_SET);

        return std::monostate{};
    }
}
