#include "TypeInferenceEngine.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../token/TokenType.hpp"
#include  <iostream>
namespace vm::compiler::types
{
    TypeInferenceEngine::TypeInferenceEngine(
        const bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const variables::VariableTracker& variableTracker,
        const variables::GlobalVariableRegistry& globalRegistry
    )
        : program(program)
        , environment(environment)
        , variableTracker(variableTracker)
        , globalRegistry(globalRegistry)
    {
    }

    value::ValueType TypeInferenceEngine::inferLiteralType(ast::ASTNode* node) const
    {
        if (dynamic_cast<ast::IntegerNode*>(node)) return value::ValueType::INT;
        if (dynamic_cast<ast::FloatNode*>(node)) return value::ValueType::FLOAT;
        if (dynamic_cast<ast::StringNode*>(node)) return value::ValueType::STRING;
        if (dynamic_cast<ast::BoolNode*>(node)) return value::ValueType::BOOL;
        if (dynamic_cast<ast::NullNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::NewNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::LambdaNode*>(node)) return value::ValueType::OBJECT;
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferVariableType(ast::VariableNode* varNode) const
    {
        std::string varName = varNode->getName();

        // Check locals
        const auto& locals = variableTracker.getLocals();
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == varName) {
                // PHASE 4: If the variable has a className that ends with [], it's an array
                if (!it->className.empty() && it->className.back() == ']') {
                    return value::ValueType::ARRAY;
                }
                return it->type;
            }
        }

        // Check globals
        if (globalRegistry.exists(varName)) {
            // PHASE 4: Check if it's an array type by class name
            std::string className = globalRegistry.getClassName(varName);
            if (!className.empty() && className.back() == ']') {
                return value::ValueType::ARRAY;
            }
            return globalRegistry.getType(varName);
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferFunctionCallType(ast::FunctionCallNode* funcCall) const
    {
        const auto* funcMetadata = program.getFunction(funcCall->getFunctionName());
        if (funcMetadata && !funcMetadata->returnType.empty()) {
            if (funcMetadata->returnType == "int") return value::ValueType::INT;
            if (funcMetadata->returnType == "float") return value::ValueType::FLOAT;
            if (funcMetadata->returnType == "string") return value::ValueType::STRING;
            if (funcMetadata->returnType == "bool") return value::ValueType::BOOL;
            return value::ValueType::OBJECT;
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferUnaryOperationType(ast::UnaryOpNode* unaryOp) const
    {
        auto operandType = inferExpressionType(unaryOp->getOperand());
        auto op = unaryOp->getOperator();

        // Unary minus and plus preserve numeric type
        if (op == token::TokenType::MINUS || op == token::TokenType::PLUS) {
            if (operandType == value::ValueType::INT || operandType == value::ValueType::FLOAT) {
                return operandType;
            }
        }

        // Logical NOT returns bool
        if (op == token::TokenType::NOT) {
            return value::ValueType::BOOL;
        }

        return operandType;
    }

    value::ValueType TypeInferenceEngine::inferCastType(ast::CastExpression* castExpr) const
    {
        const auto* targetType = castExpr->getTargetType();
        if (targetType) {
            std::string targetTypeName = targetType->toString();
            if (targetTypeName == "int") return value::ValueType::INT;
            if (targetTypeName == "float") return value::ValueType::FLOAT;
            if (targetTypeName == "string") return value::ValueType::STRING;
            if (targetTypeName == "bool") return value::ValueType::BOOL;
            return value::ValueType::OBJECT;
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferMemberAccessType(ast::MemberAccessNode* memberAccess) const
    {
        // Get the object's class name
        std::string className = inferExpressionClassName(memberAccess->getObject());
        if (!className.empty()) {
            // Look up the class definition
            auto classDef = environment->findClass(className);
            if (classDef) {
                std::string memberName = memberAccess->getMemberName();

                // Check if it's a field
                auto field = classDef->getField(memberName);
                if (field) {
                    // Check basic type first - if it's already ARRAY, return that
                    if (field->getType() == value::ValueType::ARRAY) {
                        return value::ValueType::ARRAY;
                    }

                    // Check if the field type is an array by checking the generic type
                    if (field->hasGenericType()) {
                        std::string fieldTypeName = field->getGenericType()->toString();
                        if (!fieldTypeName.empty() &&
                            (fieldTypeName.find("[]") != std::string::npos || fieldTypeName.find("Array<") == 0)) {
                            return value::ValueType::ARRAY;
                        }
                    }
                    return field->getType();
                }

                // Check if it's a method
                auto method = classDef->getMethod(memberName);
                if (method) {
                    // Check basic return type first - if it's already ARRAY, return that
                    if (method->getReturnType() == value::ValueType::ARRAY) {
                        return value::ValueType::ARRAY;
                    }

                    // Check if the return type is an array
                    if (method->getGenericReturnType()) {
                        std::string returnTypeName = method->getGenericReturnType()->toString();
                        if (!returnTypeName.empty() &&
                            (returnTypeName.find("[]") != std::string::npos || returnTypeName.find("Array<") == 0)) {
                            return value::ValueType::ARRAY;
                        }
                    }
                    return method->getReturnType();
                }
            }
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferMethodCallType(ast::MethodCallNode* methodCall) const
    {
        // Get the object's class name
        std::string className = inferExpressionClassName(methodCall->getObject());
        if (!className.empty()) {
            // Construct the fully qualified method name (ClassName::methodName)
            std::string methodName = className + "::" + methodCall->getMethodName();

            // Look up the method in the bytecode program's function registry
            const auto* funcMetadata = program.getFunction(methodName);
            if (funcMetadata && !funcMetadata->returnType.empty()) {
                if (funcMetadata->returnType == "int") return value::ValueType::INT;
                if (funcMetadata->returnType == "float") return value::ValueType::FLOAT;
                if (funcMetadata->returnType == "string") return value::ValueType::STRING;
                if (funcMetadata->returnType == "bool") return value::ValueType::BOOL;
                if (funcMetadata->returnType == "void") return value::ValueType::VOID;
                // Check if return type is an array
                if (funcMetadata->returnType.find("[]") != std::string::npos ||
                    funcMetadata->returnType.find("Array<") == 0) {
                    return value::ValueType::ARRAY;
                }
                return value::ValueType::OBJECT;
            }
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferBinaryOperationType(ast::BinaryOpNode* binOp) const
    {
        auto leftType = inferExpressionType(binOp->getLeft());
        auto rightType = inferExpressionType(binOp->getRight());
        auto op = binOp->getOperator();

        // PHASE 4: Logical operations (&&, ||) are NOT operator overloads
        // They always return primitive bool, even if operands are Bool objects (which get auto-unboxed)
        if (op == token::TokenType::AND || op == token::TokenType::OR) {
            return value::ValueType::BOOL;
        }

        // PHASE 4: Check if operator overloading applies
        // Operator overloading applies if:
        // 1. Either operand is already a Box type object, OR
        // 2. Left operand is a primitive literal that will be auto-boxed
        //
        // IMPORTANT: Operator overloading is NOT used for complex expressions (method calls, etc.)
        // to avoid complications. Only simple operands (literals, variables, member access, index access)
        // are eligible for operator overloading.

        using namespace ast::nodes::expressions;

        std::string leftClassName;
        bool willUseOperatorOverloading = false;

        // PHASE 4: Only use operator overloading if at least one operand is already a Box object
        // Don't auto-box primitive literals for operator overloading (e.g., 2 * 3 stays primitive)
        //
        // IMPORTANT: For string concatenation, prefer primitive operations over operator overloading
        // because primitive string concat handles objects via toString() without needing String class
        if (leftType == value::ValueType::OBJECT)
        {
            leftClassName = inferExpressionClassName(binOp->getLeft());
            willUseOperatorOverloading = (leftClassName == "Int" || leftClassName == "Float" ||
                                         leftClassName == "Bool" || leftClassName == "String");
        }
        else if (rightType == value::ValueType::OBJECT)
        {
            // SPECIAL CASE: Don't use operator overloading for primitive string concatenation
            // Primitive string + object already works via toString(), no String class needed
            if (leftType == value::ValueType::STRING && op == token::TokenType::PLUS)
            {
                willUseOperatorOverloading = false;  // Use primitive string concatenation
            }
            // Left is primitive but right is a Box object - we can auto-box left for operator overloading
            else if (dynamic_cast<IntegerNode*>(binOp->getLeft()))
            {
                leftClassName = "Int";
                willUseOperatorOverloading = true;
            }
            else if (dynamic_cast<FloatNode*>(binOp->getLeft()))
            {
                leftClassName = "Float";
                willUseOperatorOverloading = true;
            }
            else if (dynamic_cast<BoolNode*>(binOp->getLeft()))
            {
                leftClassName = "Bool";
                willUseOperatorOverloading = true;
            }
        }
        // else: both operands are primitives, use normal primitive operations (no operator overloading)

        // PHASE 4: Check if right operand is a "simple" expression
        // Operator overloading is only used for simple operands, not complex expressions like method calls
        bool rightIsSimple = false;
        if (rightType == value::ValueType::OBJECT)
        {
            // Right is an object - check if it's a simple expression
            if (dynamic_cast<ast::VariableNode*>(binOp->getRight()) ||
                dynamic_cast<ast::NewNode*>(binOp->getRight()) ||
                dynamic_cast<ast::MemberAccessNode*>(binOp->getRight()) ||
                dynamic_cast<IndexAccessNode*>(binOp->getRight()))
            {
                rightIsSimple = true;
            }
        }
        else
        {
            // Right is primitive - check if it's a literal
            if (dynamic_cast<IntegerNode*>(binOp->getRight()) ||
                dynamic_cast<FloatNode*>(binOp->getRight()) ||
                dynamic_cast<BoolNode*>(binOp->getRight()) ||
                dynamic_cast<StringNode*>(binOp->getRight()))
            {
                rightIsSimple = true;
            }
        }

        // If right operand is not simple, operator overloading won't be used
        if (!rightIsSimple)
        {
            willUseOperatorOverloading = false;
        }

        // If right operand is also a Box type object, operator overloading definitely applies
        if (!willUseOperatorOverloading && rightType == value::ValueType::OBJECT && rightIsSimple)
        {
            std::string rightClassName = inferExpressionClassName(binOp->getRight());
            willUseOperatorOverloading = (rightClassName == "Int" || rightClassName == "Float" ||
                                         rightClassName == "Bool" || rightClassName == "String");
        }

        if (willUseOperatorOverloading)
        {
            // Operator overloading on Box types returns OBJECT
            // For arithmetic: Int.add() returns Int object, Float.add() returns Float object
            // For comparison: Int.lessThan() returns Bool object
            // For string: String.concat() returns String object

            // PHASE 4: Equality operators (==, !=) use equals() which returns primitive bool
            // This is defined by the Object interface: function equals(T other): bool
            if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS)
            {
                return value::ValueType::BOOL;  // equals() returns primitive bool
            }

            // Comparison operators (<, <=, >, >=) use lessThan/greaterThan which return Bool objects
            if (op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS)
            {
                return value::ValueType::OBJECT;  // lessThan() etc. return Bool objects
            }

            // Arithmetic and string concatenation return OBJECT
            return value::ValueType::OBJECT;
        }

        // String concatenation with + always results in string (primitive)
        if (op == token::TokenType::PLUS &&
            (leftType == value::ValueType::STRING || rightType == value::ValueType::STRING)) {
            return value::ValueType::STRING;
        }

        // Arithmetic operations on int/float (primitives)
        if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
            op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE) {
            if (leftType == value::ValueType::FLOAT || rightType == value::ValueType::FLOAT) {
                return value::ValueType::FLOAT;
            }
            if (leftType == value::ValueType::INT && rightType == value::ValueType::INT) {
                return value::ValueType::INT;
            }
        }

        // Comparison operations return bool (primitive)
        if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS ||
            op == token::TokenType::LESS || op == token::TokenType::GREATER ||
            op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS) {
            return value::ValueType::BOOL;
        }

        // Note: Logical operations (&&, ||) are handled at the top of this function

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferIndexAccessType(ast::nodes::expressions::IndexAccessNode* indexAccess) const
    {
        // Get the class name of the collection (e.g., "Item[]", "int[]", "Array<Item>", etc.)
        std::string collectionClassName = inferExpressionClassName(indexAccess->getCollection());

        std::string elementType;

        // Check for generic Array<T> notation
        if (collectionClassName.find("Array<") == 0)
        {
            // Extract T from "Array<T>"
            size_t start = collectionClassName.find('<');
            size_t end = collectionClassName.rfind('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                elementType = collectionClassName.substr(start + 1, end - start - 1);
            }
        }
        // Check for bracket notation T[]
        else if (!collectionClassName.empty() && collectionClassName.length() >= 2 &&
                 collectionClassName.substr(collectionClassName.length() - 2) == "[]")
        {
            // Extract the element type from the array type
            elementType = collectionClassName.substr(0, collectionClassName.length() - 2);
        }

        if (!elementType.empty())
        {
            // Check if it's a primitive array
            if (elementType == "int") return value::ValueType::INT;
            if (elementType == "float") return value::ValueType::FLOAT;
            if (elementType == "string") return value::ValueType::STRING;
            if (elementType == "bool") return value::ValueType::BOOL;

            // Otherwise it's an object array
            return value::ValueType::OBJECT;
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferExpressionType(ast::ASTNode* node) const
    {
        if (!node) return value::ValueType::VOID;

        // Try to infer literal types first (most common case)
        value::ValueType literalType = inferLiteralType(node);
        if (literalType != value::ValueType::VOID) {
            return literalType;
        }

        // Delegate to specialized helpers based on node type
        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            return inferVariableType(varNode);
        }

        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            return inferFunctionCallType(funcCall);
        }

        if (auto* unaryOp = dynamic_cast<ast::UnaryOpNode*>(node)) {
            return inferUnaryOperationType(unaryOp);
        }

        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            return inferCastType(castExpr);
        }

        if (auto* memberAccess = dynamic_cast<ast::MemberAccessNode*>(node)) {
            return inferMemberAccessType(memberAccess);
        }

        if (auto* methodCall = dynamic_cast<ast::MethodCallNode*>(node)) {
            return inferMethodCallType(methodCall);
        }

        if (auto* binOp = dynamic_cast<ast::BinaryOpNode*>(node)) {
            return inferBinaryOperationType(binOp);
        }

        if (auto* indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node)) {
            return inferIndexAccessType(indexAccess);
        }

        // Handle await expressions - unwrap Promise<T> to get T
        if (auto* awaitExpr = dynamic_cast<ast::nodes::expressions::AwaitExpression*>(node)) {
            // The await expression evaluates the inner expression and unwraps the Promise
            // For Promise<Int>, await returns Int (OBJECT type)
            // For Promise<int>, await returns int (INT type) - but this is non-standard
            // Since async functions return Promise<T> where T is a wrapper class, await returns OBJECT

            // Get the type of the expression being awaited
            auto* innerExpr = awaitExpr->getExpressionPtr();
            if (innerExpr) {
                // If it's a function call returning Promise<T>, we need to unwrap to T
                // For now, we'll infer the type of the inner expression
                // In most cases, async functions return Promise<WrapperClass>, so the result is OBJECT
                auto innerType = inferExpressionType(innerExpr);

                // If the inner expression returns an OBJECT (Promise), the await unwraps it
                // The unwrapped value is still an OBJECT (the wrapper class like Int, String, etc.)
                if (innerType == value::ValueType::OBJECT) {
                    return value::ValueType::OBJECT;
                }

                // For other types, return as-is (though this shouldn't normally happen)
                return innerType;
            }
        }

        return value::ValueType::VOID;
    }

    std::string TypeInferenceEngine::inferCastClassName(ast::CastExpression* castExpr) const
    {
        const auto* targetType = castExpr->getTargetType();
        if (targetType) {
            std::string targetTypeName = targetType->toString();
            // Only return class name if it's not a primitive type
            if (targetTypeName != "int" && targetTypeName != "float" &&
                targetTypeName != "string" && targetTypeName != "bool" &&
                targetTypeName != "void") {
                return targetTypeName;
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferVariableClassName(ast::VariableNode* varNode) const
    {
        std::string varName = varNode->getName();

        // Check locals
        const auto& locals = variableTracker.getLocals();
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == varName) {
                return it->className;
            }
        }

        // Check globals
        if (globalRegistry.exists(varName)) {
            return globalRegistry.getClassName(varName);
        }

        return "";
    }

    std::string TypeInferenceEngine::inferFunctionCallClassName(ast::FunctionCallNode* funcCall) const
    {
        std::string functionName = funcCall->getFunctionName();
        const auto* funcMetadata = program.getFunction(functionName);

        // If this is a generic function call, resolve the return type using the provided type arguments
        if (funcCall->hasGenericTypeArguments() && funcMetadata && !funcMetadata->genericTypeParameters.empty())
        {
            const auto& genericTypeArgs = funcCall->getGenericTypeArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;
            std::string returnType = funcMetadata->returnType;

            // Build temporary bindings for this function call
            for (size_t i = 0; i < genericTypeParams.size() && i < genericTypeArgs.size(); ++i)
            {
                if (returnType == genericTypeParams[i])
                {
                    // The return type is a generic parameter, substitute it
                    return genericTypeArgs[i];
                }
            }
        }

        if (funcMetadata && !funcMetadata->returnType.empty()) {
            // If return type is not a primitive, it's a class name
            if (funcMetadata->returnType != "int" && funcMetadata->returnType != "float" &&
                funcMetadata->returnType != "string" && funcMetadata->returnType != "bool" &&
                funcMetadata->returnType != "void" && funcMetadata->returnType != "object") {
                // Resolve generic type if applicable (from context stack)
                return resolveGenericType(funcMetadata->returnType);
            }
        }

        // Also check in the environment for functions
        auto funcDef = environment->findFunction(functionName);
        if (funcDef) {
            std::string returnClassName = funcDef->getReturnClassName();
            if (!returnClassName.empty()) {
                // Resolve generic type if applicable
                return resolveGenericType(returnClassName);
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferIndexAccessClassName(ast::nodes::expressions::IndexAccessNode* indexAccess) const
    {
        // Get the class name of the collection (e.g., "Item[]", "Array<Item>", "List<String>[]", etc.)
        std::string collectionClassName = inferExpressionClassName(indexAccess->getCollection());

        std::string elementType;

        // Check for generic Array<T> notation
        if (collectionClassName.find("Array<") == 0)
        {
            // Extract T from "Array<T>"
            size_t start = collectionClassName.find('<');
            size_t end = collectionClassName.rfind('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                elementType = collectionClassName.substr(start + 1, end - start - 1);
            }
        }
        // Check for bracket notation T[]
        else if (!collectionClassName.empty() && collectionClassName.length() >= 2 &&
                 collectionClassName.substr(collectionClassName.length() - 2) == "[]")
        {
            // Extract the element type from the array type (e.g., "Item[]" -> "Item", "List<String>[]" -> "List<String>")
            elementType = collectionClassName.substr(0, collectionClassName.length() - 2);
        }

        if (!elementType.empty())
        {
            // Don't return primitive type names as class names
            if (elementType != "int" && elementType != "float" &&
                elementType != "string" && elementType != "bool" &&
                elementType != "void")
            {
                return elementType;
            }
        }

        return "";
    }

    std::string TypeInferenceEngine::inferMemberAccessClassName(ast::MemberAccessNode* memberAccess) const
    {
        // Get the object's class name
        std::string className = inferExpressionClassName(memberAccess->getObject());
        if (!className.empty()) {
            // Look up the class definition
            auto classDef = environment->findClass(className);
            if (classDef) {
                std::string memberName = memberAccess->getMemberName();

                // Check if it's a field
                auto field = classDef->getField(memberName);
                if (field) {
                    if (field->hasGenericType()) {
                        std::string fieldTypeName = field->getGenericType()->toString();
                        // Don't return primitive type names as class names
                        if (fieldTypeName != "int" && fieldTypeName != "float" &&
                            fieldTypeName != "string" && fieldTypeName != "bool" &&
                            fieldTypeName != "void") {
                            return fieldTypeName;
                        }
                    }
                }

                // Check if it's a method
                auto method = classDef->getMethod(memberName);
                if (method) {
                    if (method->getGenericReturnType()) {
                        std::string returnTypeName = method->getGenericReturnType()->toString();
                        // Don't return primitive type names as class names
                        if (returnTypeName != "int" && returnTypeName != "float" &&
                            returnTypeName != "string" && returnTypeName != "bool" &&
                            returnTypeName != "void") {
                            return returnTypeName;
                        }
                    }
                }
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferMethodCallClassName(ast::MethodCallNode* methodCall) const
    {
        // Get the object's class name
        std::string className = inferExpressionClassName(methodCall->getObject());
        if (!className.empty()) {
            // Construct the fully qualified method name (ClassName::methodName)
            std::string methodName = className + "::" + methodCall->getMethodName();

            // Look up the method in the bytecode program's function registry
            const auto* funcMetadata = program.getFunction(methodName);
            if (funcMetadata && !funcMetadata->returnType.empty()) {
                // Don't return primitive type names as class names
                if (funcMetadata->returnType != "int" && funcMetadata->returnType != "float" &&
                    funcMetadata->returnType != "string" && funcMetadata->returnType != "bool" &&
                    funcMetadata->returnType != "void") {
                    return funcMetadata->returnType;
                }
            }
        }
        return "";
    }

    std::string TypeInferenceEngine::inferExpressionClassName(ast::ASTNode* node) const
    {
        if (!node) return "";

        // Cast expressions
        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            return inferCastClassName(castExpr);
        }

        // NewNode
        if (auto* newNode = dynamic_cast<ast::NewNode*>(node)) {
            return newNode->getClassName();
        }

        // Variable references
        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            return inferVariableClassName(varNode);
        }

        // Function calls
        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            return inferFunctionCallClassName(funcCall);
        }

        // Index access (array element access)
        if (auto* indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node)) {
            return inferIndexAccessClassName(indexAccess);
        }

        // Member access (field or method)
        if (auto* memberAccess = dynamic_cast<ast::MemberAccessNode*>(node)) {
            return inferMemberAccessClassName(memberAccess);
        }

        // Method calls
        if (auto* methodCall = dynamic_cast<ast::MethodCallNode*>(node)) {
            return inferMethodCallClassName(methodCall);
        }

        // PHASE 4: Binary operations with operator overloading
        if (auto* binOp = dynamic_cast<ast::BinaryOpNode*>(node)) {
            using namespace ast::nodes::expressions;

            auto leftType = inferExpressionType(binOp->getLeft());
            auto rightType = inferExpressionType(binOp->getRight());
            auto op = binOp->getOperator();

            // PHASE 4: Only return className if operator overloading is actually used
            // Match the logic in inferBinaryOperationType for consistency

            // Determine left class name
            std::string leftClassName;
            bool willUseOperatorOverloading = false;

            if (leftType == value::ValueType::OBJECT)
            {
                leftClassName = inferExpressionClassName(binOp->getLeft());
                willUseOperatorOverloading = (leftClassName == "Int" || leftClassName == "Float" ||
                                             leftClassName == "Bool" || leftClassName == "String");
            }
            else if (rightType == value::ValueType::OBJECT)
            {
                // SPECIAL CASE: Don't use operator overloading for primitive string concatenation
                if (leftType == value::ValueType::STRING && op == token::TokenType::PLUS)
                {
                    willUseOperatorOverloading = false;  // Primitive string concat, no Box class
                }
                // Left is primitive but right is a Box object - check for operator overloading
                else if (dynamic_cast<IntegerNode*>(binOp->getLeft()))
                {
                    leftClassName = "Int";
                    willUseOperatorOverloading = true;
                }
                else if (dynamic_cast<FloatNode*>(binOp->getLeft()))
                {
                    leftClassName = "Float";
                    willUseOperatorOverloading = true;
                }
                else if (dynamic_cast<BoolNode*>(binOp->getLeft()))
                {
                    leftClassName = "Bool";
                    willUseOperatorOverloading = true;
                }
            }

            // If not using operator overloading, return empty (result is primitive, not object)
            if (!willUseOperatorOverloading)
            {
                return "";
            }

            // Check if this is a Box type operation
            bool isBoxTypeOp = (leftClassName == "Int" || leftClassName == "Float" ||
                               leftClassName == "Bool" || leftClassName == "String");

            if (isBoxTypeOp)
            {
                // PHASE 4: Equality operators (==, !=) return primitive bool (via equals())
                if (op == token::TokenType::EQUALS || op == token::TokenType::NOT_EQUALS)
                {
                    return "";  // equals() returns primitive bool, not Bool object
                }

                // Comparison operators (<, <=, >, >=) return Bool objects
                if (op == token::TokenType::LESS || op == token::TokenType::GREATER ||
                    op == token::TokenType::LESS_EQUALS || op == token::TokenType::GREATER_EQUALS)
                {
                    return "Bool";  // lessThan() etc. return Bool objects
                }

                // For arithmetic/concatenation operators, result is same type as left operand
                if (op == token::TokenType::PLUS || op == token::TokenType::MINUS ||
                    op == token::TokenType::MULTIPLY || op == token::TokenType::DIVIDE ||
                    op == token::TokenType::MODULO)
                {
                    return leftClassName;  // Returns same Box type as left operand
                }
            }

            return "";  // Primitive operations don't have class names
        }

        // Await expressions - unwrap Promise<T> to get T's class name
        if (auto* awaitExpr = dynamic_cast<ast::nodes::expressions::AwaitExpression*>(node)) {
            auto* innerExpr = awaitExpr->getExpressionPtr();
            if (innerExpr) {
                // If the inner expression is a function call, get its return type
                if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(awaitExpr->getExpressionPtr())) {
                    std::string className = inferFunctionCallClassName(funcCall);

                    // If the class name is Promise<T>, extract T
                    if (className.find("Promise<") == 0) {
                        size_t start = className.find('<');
                        size_t end = className.rfind('>');
                        if (start != std::string::npos && end != std::string::npos && end > start) {
                            // Extract T from Promise<T>
                            std::string unwrappedType = className.substr(start + 1, end - start - 1);
                            return unwrappedType;
                        }
                    }

                    return className;
                }

                // For other expressions, try to infer the class name
                std::string innerClassName = inferExpressionClassName(innerExpr);

                // If the inner class is Promise<T>, unwrap it
                if (innerClassName.find("Promise<") == 0) {
                    size_t start = innerClassName.find('<');
                    size_t end = innerClassName.rfind('>');
                    if (start != std::string::npos && end != std::string::npos && end > start) {
                        // Extract T from Promise<T>
                        std::string unwrappedType = innerClassName.substr(start + 1, end - start - 1);
                        return unwrappedType;
                    }
                }

                return innerClassName;
            }
        }

        return "";
    }

    void TypeInferenceEngine::setGenericTypeBindingsStack(
        const std::vector<std::unordered_map<std::string, std::string>>* stack)
    {
        genericTypeBindingsStack = stack;
    }

    std::string TypeInferenceEngine::resolveGenericType(const std::string& typeName) const
    {
        if (!genericTypeBindingsStack || genericTypeBindingsStack->empty())
        {
            return typeName;
        }

        // Check from most recent to oldest binding context
        for (auto it = genericTypeBindingsStack->rbegin(); it != genericTypeBindingsStack->rend(); ++it)
        {
            auto found = it->find(typeName);
            if (found != it->end())
            {
                return found->second;
            }
        }

        // Not a generic type parameter, return as-is
        return typeName;
    }
}
