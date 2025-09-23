#include "CompleteJSONSerializer.hpp"
#include <fstream>
#include <iostream>

// For import resolution
#include "../../services/ImportManager.hpp"
#include "../../parser/Parser.hpp"
#include "../../lexer/Lexer.hpp"

// Include all the AST node types
#include "../nodes/expressions/IntegerNode.hpp"
#include "../nodes/expressions/FloatNode.hpp"
#include "../nodes/expressions/StringNode.hpp"
#include "../nodes/expressions/BoolNode.hpp"
#include "../nodes/expressions/NullNode.hpp"
#include "../nodes/expressions/VariableNode.hpp"
#include "../nodes/expressions/BinaryExpNode.hpp"
#include "../nodes/expressions/UnaryExpNode.hpp"
#include "../nodes/expressions/TernaryExpNode.hpp"
#include "../nodes/expressions/IndexAccessNode.hpp"
#include "../nodes/expressions/ArrayCreationNode.hpp"

#include "../nodes/statements/ProgramNode.hpp"
#include "../nodes/statements/BlockNode.hpp"
#include "../nodes/statements/DeclarationNode.hpp"
#include "../nodes/statements/AssignmentNode.hpp"
#include "../nodes/statements/MemberAssignmentNode.hpp"
#include "../nodes/statements/IndexAssignmentNode.hpp"
#include "../nodes/statements/ImportNode.hpp"
#include "../nodes/statements/IfNode.hpp"
#include "../nodes/statements/WhileNode.hpp"
#include "../nodes/statements/DoWhileNode.hpp"
#include "../nodes/statements/ForNode.hpp"
#include "../nodes/statements/ForEachNode.hpp"
#include "../nodes/statements/BreakNode.hpp"
#include "../nodes/statements/ContinueNode.hpp"
#include "../nodes/statements/SwitchNode.hpp"
#include "../nodes/statements/CaseNode.hpp"
#include "../nodes/statements/DefaultCaseNode.hpp"

#include "../nodes/functions/FunctionNode.hpp"
#include "../nodes/functions/FunctionCallNode.hpp"
#include "../nodes/functions/ReturnNode.hpp"

#include "../nodes/classes/ClassNode.hpp"
#include "../nodes/classes/NewNode.hpp"
#include "../nodes/classes/MemberAccessNode.hpp"
#include "../nodes/classes/MethodCallNode.hpp"
#include "../nodes/classes/FieldNode.hpp"
#include "../nodes/classes/MethodNode.hpp"
#include "../nodes/classes/ConstructorNode.hpp"

namespace ast::serialization
{
    CompleteJSONSerializer::CompleteJSONSerializer()
    {
    }

    // Helper method to add source location to JSON
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeSourceLocation(const ASTNode* node)
    {
        json locationJson;
        if (node) {
            const auto& location = node->getLocation();
            locationJson["line"] = location.getLine();
            locationJson["column"] = location.getColumn();
        }
        return locationJson;
    }

    // Main serialization method
    std::string CompleteJSONSerializer::serialize(const std::shared_ptr<ASTNode>& root)
    {
        if (!root) {
            return "null";
        }

        json rootJson = serializeNode(root.get());
        return rootJson.dump(2); // Pretty print with 2-space indentation
    }

    // Serialize AST and save to file
    bool CompleteJSONSerializer::serializeToFile(const std::shared_ptr<ASTNode>& root, const std::string& filename)
    {
        try {
            std::string jsonString = serialize(root);

            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                return false;
            }

            file << jsonString;
            file.close();

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error serializing to file " << filename << ": " << e.what() << std::endl;
            return false;
        }
    }

    // Core node serialization dispatcher
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeNode(ASTNode* node)
    {
        if (!node) {
            return nullptr;
        }

        NodeType nodeType = getNodeType(node);
        json nodeJson;

        // Add common fields
        nodeJson["nodeType"] = getNodeTypeName(nodeType);
        nodeJson["nodeTypeValue"] = static_cast<int>(nodeType);

        // Add source location
        const auto& location = node->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        // Dispatch to specific serializer based on node type
        switch (nodeType) {
            // Expression nodes
            case NodeType::INTEGER_NODE:
                return serializeIntegerNode(node);
            case NodeType::FLOAT_NODE:
                return serializeFloatNode(node);
            case NodeType::STRING_NODE:
                return serializeStringNode(node);
            case NodeType::BOOL_NODE:
                return serializeBoolNode(node);
            case NodeType::NULL_NODE:
                return serializeNullNode(node);
            case NodeType::VARIABLE_NODE:
                return serializeVariableNode(node);
            case NodeType::BINARY_EXP_NODE:
                return serializeBinaryExpNode(node);
            case NodeType::UNARY_EXP_NODE:
                return serializeUnaryExpNode(node);
            case NodeType::TERNARY_EXP_NODE:
                return serializeTernaryExpNode(node);
            case NodeType::INDEX_ACCESS_NODE:
                return serializeIndexAccessNode(node);
            case NodeType::ARRAY_CREATION_NODE:
                return serializeArrayCreationNode(node);

            // Statement nodes
            case NodeType::PROGRAM_NODE:
                return serializeProgramNode(node);
            case NodeType::BLOCK_NODE:
                return serializeBlockNode(node);
            case NodeType::DECLARATION_NODE:
                return serializeDeclarationNode(node);
            case NodeType::ASSIGNMENT_NODE:
                return serializeAssignmentNode(node);
            case NodeType::MEMBER_ASSIGNMENT_NODE:
                return serializeMemberAssignmentNode(node);
            case NodeType::INDEX_ASSIGNMENT_NODE:
                return serializeIndexAssignmentNode(node);
            case NodeType::IF_NODE:
                return serializeIfNode(node);
            case NodeType::WHILE_NODE:
                return serializeWhileNode(node);
            case NodeType::DO_WHILE_NODE:
                return serializeDoWhileNode(node);
            case NodeType::FOR_NODE:
                return serializeForNode(node);
            case NodeType::FOR_EACH_NODE:
                return serializeForEachNode(node);
            case NodeType::BREAK_NODE:
                return serializeBreakNode(node);
            case NodeType::CONTINUE_NODE:
                return serializeContinueNode(node);
            case NodeType::SWITCH_NODE:
                return serializeSwitchNode(node);
            case NodeType::CASE_NODE:
                return serializeCaseNode(node);
            case NodeType::DEFAULT_CASE_NODE:
                return serializeDefaultCaseNode(node);
            case NodeType::IMPORT_NODE:
                return serializeImportNode(node);

            // Function nodes
            case NodeType::FUNCTION_NODE:
                return serializeFunctionNode(node);
            case NodeType::FUNCTION_CALL_NODE:
                return serializeFunctionCallNode(node);
            case NodeType::RETURN_NODE:
                return serializeReturnNode(node);

            // Class nodes
            case NodeType::CLASS_NODE:
                return serializeClassNode(node);
            case NodeType::NEW_NODE:
                return serializeNewNode(node);
            case NodeType::MEMBER_ACCESS_NODE:
                return serializeMemberAccessNode(node);
            case NodeType::METHOD_CALL_NODE:
                return serializeMethodCallNode(node);
            case NodeType::FIELD_NODE:
                return serializeFieldNode(node);
            case NodeType::METHOD_NODE:
                return serializeMethodNode(node);
            case NodeType::CONSTRUCTOR_NODE:
                return serializeConstructorNode(node);

            default:
                std::cerr << "Unknown node type: " << static_cast<int>(nodeType) << std::endl;
                return nodeJson; // Return basic node with just common fields
        }
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeNode(const std::shared_ptr<ASTNode>& node)
    {
        return serializeNode(node.get());
    }

    // Expression node serializers
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeIntegerNode(const ASTNode* node)
    {
        auto intNode = dynamic_cast<const nodes::expressions::IntegerNode*>(node);
        if (!intNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "IntegerNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::INTEGER_NODE);
        nodeJson["value"] = intNode->getValue();

        const auto& location = intNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeFloatNode(const ASTNode* node)
    {
        auto floatNode = dynamic_cast<const nodes::expressions::FloatNode*>(node);
        if (!floatNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "FloatNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FLOAT_NODE);
        nodeJson["value"] = floatNode->getValue();

        const auto& location = floatNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeStringNode(const ASTNode* node)
    {
        auto stringNode = dynamic_cast<const nodes::expressions::StringNode*>(node);
        if (!stringNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "StringNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::STRING_NODE);
        nodeJson["value"] = stringNode->getValue();

        const auto& location = stringNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeBoolNode(const ASTNode* node)
    {
        auto boolNode = dynamic_cast<const nodes::expressions::BoolNode*>(node);
        if (!boolNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "BoolNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::BOOL_NODE);
        nodeJson["value"] = boolNode->getValue();

        const auto& location = boolNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeNullNode(const ASTNode* node)
    {
        auto nullNode = dynamic_cast<const nodes::expressions::NullNode*>(node);
        if (!nullNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "NullNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::NULL_NODE);

        const auto& location = nullNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeVariableNode(const ASTNode* node)
    {
        auto varNode = dynamic_cast<const nodes::expressions::VariableNode*>(node);
        if (!varNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "VariableNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::VARIABLE_NODE);
        nodeJson["name"] = varNode->getName();

        const auto& location = varNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeBinaryExpNode(const ASTNode* node)
    {
        auto binNode = dynamic_cast<const nodes::expressions::BinaryExpNode*>(node);
        if (!binNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "BinaryExpNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::BINARY_EXP_NODE);
        nodeJson["operator"] = static_cast<int>(binNode->getOperator());
        nodeJson["left"] = serializeNode(binNode->getLeft());
        nodeJson["right"] = serializeNode(binNode->getRight());

        const auto& location = binNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeUnaryExpNode(const ASTNode* node)
    {
        auto unaryNode = dynamic_cast<const nodes::expressions::UnaryExpNode*>(node);
        if (!unaryNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "UnaryExpNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::UNARY_EXP_NODE);
        nodeJson["operator"] = static_cast<int>(unaryNode->getOperator());
        nodeJson["operand"] = serializeNode(unaryNode->getOperand());
        nodeJson["isPrefix"] = unaryNode->isPrefix();

        const auto& location = unaryNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    // Statement node serializers
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeProgramNode(const ASTNode* node)
    {
        auto progNode = dynamic_cast<const nodes::statements::ProgramNode*>(node);
        if (!progNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "ProgramNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::PROGRAM_NODE);

        json statementsArray = json::array();
        for (const auto& stmt : progNode->getStatements()) {
            if (stmt) {
                statementsArray.push_back(serializeNode(stmt.get()));
            }
        }
        nodeJson["statements"] = statementsArray;

        const auto& location = progNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeBlockNode(const ASTNode* node)
    {
        auto blockNode = dynamic_cast<const nodes::statements::BlockNode*>(node);
        if (!blockNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "BlockNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::BLOCK_NODE);

        json statementsArray = json::array();
        for (const auto& stmt : blockNode->getStatements()) {
            if (stmt) {
                statementsArray.push_back(serializeNode(stmt.get()));
            }
        }
        nodeJson["statements"] = statementsArray;

        const auto& location = blockNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    // Function node serializers
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeFunctionCallNode(const ASTNode* node)
    {
        auto funcCallNode = dynamic_cast<const nodes::functions::FunctionCallNode*>(node);
        if (!funcCallNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "FunctionCallNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FUNCTION_CALL_NODE);
        nodeJson["functionName"] = funcCallNode->getFunctionName();

        json argsArray = json::array();
        for (const auto& arg : funcCallNode->getArguments()) {
            if (arg) {
                argsArray.push_back(serializeNode(arg.get()));
            }
        }
        nodeJson["arguments"] = argsArray;

        const auto& location = funcCallNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeReturnNode(const ASTNode* node)
    {
        auto returnNode = dynamic_cast<const nodes::functions::ReturnNode*>(node);
        if (!returnNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "ReturnNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::RETURN_NODE);

        if (returnNode->getReturnValue()) {
            nodeJson["value"] = serializeNode(returnNode->getReturnValue());
        } else {
            nodeJson["value"] = nullptr;
        }

        const auto& location = returnNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    // Placeholder implementations for remaining node types
    // These will be implemented based on the specific node structures

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeTernaryExpNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "TernaryExpNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::TERNARY_EXP_NODE);
        // TODO: Implement based on TernaryExpNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeIndexAccessNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "IndexAccessNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::INDEX_ACCESS_NODE);
        // TODO: Implement based on IndexAccessNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeArrayCreationNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "ArrayCreationNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::ARRAY_CREATION_NODE);
        // TODO: Implement based on ArrayCreationNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeDeclarationNode(const ASTNode* node)
    {
        auto declNode = dynamic_cast<const nodes::statements::DeclarationNode*>(node);
        if (!declNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "DeclarationNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::DECLARATION_NODE);
        nodeJson["variableName"] = declNode->getVariableName();
        nodeJson["type"] = static_cast<int>(declNode->getType());
        nodeJson["isFinal"] = declNode->isFinal();
        nodeJson["isStatic"] = declNode->isStatic();

        // Serialize the initializer
        if (declNode->getInitializer()) {
            nodeJson["initializer"] = serializeNode(declNode->getInitializer());
        } else {
            nodeJson["initializer"] = nullptr;
        }

        const auto& location = declNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeAssignmentNode(const ASTNode* node)
    {
        auto assignNode = dynamic_cast<const nodes::statements::AssignmentNode*>(node);
        if (!assignNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "AssignmentNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::ASSIGNMENT_NODE);
        nodeJson["variableName"] = assignNode->getVariableName();
        nodeJson["variableType"] = static_cast<int>(assignNode->getVariableType());
        nodeJson["className"] = assignNode->getClassName();
        nodeJson["isFinal"] = assignNode->getIsFinal();
        nodeJson["isStatic"] = assignNode->getIsStatic();

        // Serialize the value
        if (assignNode->getValue()) {
            nodeJson["value"] = serializeNode(assignNode->getValue());
        } else {
            nodeJson["value"] = nullptr;
        }

        const auto& location = assignNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeMemberAssignmentNode(const ASTNode* node)
    {
        auto memberAssignNode = dynamic_cast<const nodes::statements::MemberAssignmentNode*>(node);
        if (!memberAssignNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "MemberAssignmentNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::MEMBER_ASSIGNMENT_NODE);
        nodeJson["memberName"] = memberAssignNode->getMemberName();

        // Serialize the object
        if (memberAssignNode->getObject()) {
            nodeJson["object"] = serializeNode(memberAssignNode->getObject());
        } else {
            nodeJson["object"] = nullptr;
        }

        // Serialize the value
        if (memberAssignNode->getValue()) {
            nodeJson["value"] = serializeNode(memberAssignNode->getValue());
        } else {
            nodeJson["value"] = nullptr;
        }

        const auto& location = memberAssignNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeIndexAssignmentNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "IndexAssignmentNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::INDEX_ASSIGNMENT_NODE);
        // TODO: Implement based on IndexAssignmentNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeIfNode(const ASTNode* node)
    {
        auto ifNode = dynamic_cast<const nodes::statements::IfNode*>(node);
        if (!ifNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "IfNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::IF_NODE);
        nodeJson["hasElseStatement"] = ifNode->hasElseStatement();

        // Serialize condition
        if (ifNode->getCondition()) {
            nodeJson["condition"] = serializeNode(ifNode->getCondition());
        } else {
            nodeJson["condition"] = nullptr;
        }

        // Serialize then statement
        if (ifNode->getThenStatement()) {
            nodeJson["thenStatement"] = serializeNode(ifNode->getThenStatement());
        } else {
            nodeJson["thenStatement"] = nullptr;
        }

        // Serialize else statement
        if (ifNode->getElseStatement()) {
            nodeJson["elseStatement"] = serializeNode(ifNode->getElseStatement());
        } else {
            nodeJson["elseStatement"] = nullptr;
        }

        const auto& location = ifNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeWhileNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "WhileNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::WHILE_NODE);
        // TODO: Implement based on WhileNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeDoWhileNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "DoWhileNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::DO_WHILE_NODE);
        // TODO: Implement based on DoWhileNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeForNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "ForNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FOR_NODE);
        // TODO: Implement based on ForNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeForEachNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "ForEachNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FOR_EACH_NODE);
        // TODO: Implement based on ForEachNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeBreakNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "BreakNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::BREAK_NODE);
        // TODO: Implement based on BreakNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeContinueNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "ContinueNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::CONTINUE_NODE);
        // TODO: Implement based on ContinueNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeSwitchNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "SwitchNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::SWITCH_NODE);
        // TODO: Implement based on SwitchNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeCaseNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "CaseNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::CASE_NODE);
        // TODO: Implement based on CaseNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeDefaultCaseNode(const ASTNode* node)
    {
        json nodeJson;
        nodeJson["nodeType"] = "DefaultCaseNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::DEFAULT_CASE_NODE);
        // TODO: Implement based on DefaultCaseNode structure
        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeImportNode(const ASTNode* node)
    {
        auto importNode = dynamic_cast<const nodes::statements::ImportNode*>(node);
        if (!importNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "ImportNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::IMPORT_NODE);
        nodeJson["filePath"] = importNode->getFilePath();
        nodeJson["isResolved"] = importNode->isResolved();

        // Serialize imported declarations if they exist
        json declarationsArray = json::array();
        for (const auto& decl : importNode->getImportedDeclarations()) {
            if (decl) {
                declarationsArray.push_back(serializeNode(decl.get()));
            }
        }
        nodeJson["importedDeclarations"] = declarationsArray;

        // If the import is resolved and has an imported AST, serialize it
        if (importNode->isResolved() && importNode->getImportedAST()) {
            nodeJson["importedAST"] = serializeNode(importNode->getImportedAST());
        } else {
            // Try to load and serialize the imported file if possible
            std::string resolvedPath = resolveImportPath(importNode->getFilePath());
            auto importedAST = loadImportedAST(resolvedPath);
            if (importedAST) {
                nodeJson["importedAST"] = serializeNode(importedAST.get());
            } else {
                nodeJson["importedAST"] = nullptr;
            }
        }

        const auto& location = importNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeFunctionNode(const ASTNode* node)
    {
        auto funcNode = dynamic_cast<const nodes::functions::FunctionNode*>(node);
        if (!funcNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "FunctionNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FUNCTION_NODE);
        nodeJson["name"] = funcNode->getName();
        nodeJson["returnType"] = static_cast<int>(funcNode->getReturnType());

        // Serialize parameters
        json paramsArray = json::array();
        for (const auto& param : funcNode->getParameters()) {
            json paramObj;
            paramObj["name"] = param.first;
            paramObj["type"] = static_cast<int>(param.second);
            paramsArray.push_back(paramObj);
        }
        nodeJson["parameters"] = paramsArray;

        // Serialize body
        if (funcNode->getBodyPtr()) {
            nodeJson["body"] = serializeNode(funcNode->getBodyPtr());
        } else {
            nodeJson["body"] = nullptr;
        }

        const auto& location = funcNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeClassNode(const ASTNode* node)
    {
        auto classNode = dynamic_cast<const nodes::classes::ClassNode*>(node);
        if (!classNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "ClassNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::CLASS_NODE);
        nodeJson["className"] = classNode->getClassName();
        nodeJson["isGeneric"] = classNode->isGeneric();

        // Serialize generic parameters
        json genericParams = json::array();
        for (const auto& param : classNode->getGenericParameters()) {
            json paramObj;
            paramObj["name"] = param.name;
            paramObj["hasConstraints"] = param.hasConstraints();
            // Serialize constraints
            json constraintsArray = json::array();
            for (const auto& constraint : param.constraints) {
                constraintsArray.push_back(constraint);
            }
            paramObj["constraints"] = constraintsArray;
            genericParams.push_back(paramObj);
        }
        nodeJson["genericParameters"] = genericParams;

        // Serialize fields
        json fieldsArray = json::array();
        for (const auto& field : classNode->getFields()) {
            if (field) {
                fieldsArray.push_back(serializeNode(field.get()));
            }
        }
        nodeJson["fields"] = fieldsArray;

        // Serialize constructors
        json constructorsArray = json::array();
        for (const auto& constructor : classNode->getConstructors()) {
            if (constructor) {
                constructorsArray.push_back(serializeNode(constructor.get()));
            }
        }
        nodeJson["constructors"] = constructorsArray;

        // Serialize methods
        json methodsArray = json::array();
        for (const auto& method : classNode->getMethods()) {
            if (method) {
                methodsArray.push_back(serializeNode(method.get()));
            }
        }
        nodeJson["methods"] = methodsArray;

        const auto& location = classNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeNewNode(const ASTNode* node)
    {
        auto newNode = dynamic_cast<const nodes::classes::NewNode*>(node);
        if (!newNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "NewNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::NEW_NODE);
        nodeJson["className"] = newNode->getClassName();

        // Serialize arguments
        json argsArray = json::array();
        for (const auto& arg : newNode->getArguments()) {
            if (arg) {
                argsArray.push_back(serializeNode(arg.get()));
            }
        }
        nodeJson["arguments"] = argsArray;

        const auto& location = newNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeMemberAccessNode(const ASTNode* node)
    {
        auto memberAccessNode = static_cast<const nodes::classes::MemberAccessNode*>(node);

        json nodeJson;
        nodeJson["nodeType"] = "MemberAccessNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::MEMBER_ACCESS_NODE);

        // Serialize the base location information
        auto location = memberAccessNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        // Serialize the object being accessed
        if (memberAccessNode->getObject()) {
            nodeJson["object"] = serializeNode(memberAccessNode->getObject());
        }

        // Serialize member name and static access flag
        nodeJson["memberName"] = memberAccessNode->getMemberName();
        nodeJson["isStaticAccess"] = memberAccessNode->getIsStaticAccess();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeMethodCallNode(const ASTNode* node)
    {
        auto methodCallNode = dynamic_cast<const nodes::classes::MethodCallNode*>(node);
        if (!methodCallNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "MethodCallNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::METHOD_CALL_NODE);
        nodeJson["methodName"] = methodCallNode->getMethodName();
        nodeJson["isStaticCall"] = methodCallNode->getIsStaticCall();
        nodeJson["hasGenericTypeArguments"] = methodCallNode->hasGenericTypeArguments();

        // Serialize the object (null for static calls)
        if (methodCallNode->getObject()) {
            nodeJson["object"] = serializeNode(methodCallNode->getObject());
        } else {
            nodeJson["object"] = nullptr;
        }

        // Serialize arguments
        json argsArray = json::array();
        for (const auto& arg : methodCallNode->getArguments()) {
            if (arg) {
                argsArray.push_back(serializeNode(arg.get()));
            }
        }
        nodeJson["arguments"] = argsArray;

        // Serialize generic type arguments
        json genericArgsArray = json::array();
        for (const auto& genericArg : methodCallNode->getGenericTypeArguments()) {
            genericArgsArray.push_back(genericArg);
        }
        nodeJson["genericTypeArguments"] = genericArgsArray;

        const auto& location = methodCallNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeFieldNode(const ASTNode* node)
    {
        auto fieldNode = dynamic_cast<const nodes::classes::FieldNode*>(node);
        if (!fieldNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "FieldNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::FIELD_NODE);
        nodeJson["name"] = fieldNode->getName();
        nodeJson["type"] = static_cast<int>(fieldNode->getType());
        nodeJson["isStatic"] = fieldNode->getIsStatic();
        nodeJson["isFinal"] = fieldNode->getIsFinal();
        nodeJson["hasInitialValue"] = fieldNode->hasInitialValue();

        // Serialize initial value
        if (fieldNode->getInitialValue()) {
            nodeJson["initialValue"] = serializeNode(fieldNode->getInitialValue());
        } else {
            nodeJson["initialValue"] = nullptr;
        }

        const auto& location = fieldNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeMethodNode(const ASTNode* node)
    {
        auto methodNode = dynamic_cast<const nodes::classes::MethodNode*>(node);
        if (!methodNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "MethodNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::METHOD_NODE);
        nodeJson["name"] = methodNode->getName();
        nodeJson["returnType"] = static_cast<int>(methodNode->getReturnType());
        nodeJson["isStatic"] = methodNode->getIsStatic();
        nodeJson["isGeneric"] = methodNode->isGeneric();

        // Serialize parameters
        json paramsArray = json::array();
        for (const auto& param : methodNode->getParameters()) {
            json paramObj;
            paramObj["name"] = param.first;
            paramObj["type"] = static_cast<int>(param.second);
            paramsArray.push_back(paramObj);
        }
        nodeJson["parameters"] = paramsArray;

        // Serialize generic type parameters
        json genericParams = json::array();
        for (const auto& param : methodNode->getGenericTypeParameters()) {
            json paramObj;
            paramObj["name"] = param.name;
            paramObj["hasConstraints"] = param.hasConstraints();
            json constraintsArray = json::array();
            for (const auto& constraint : param.constraints) {
                constraintsArray.push_back(constraint);
            }
            paramObj["constraints"] = constraintsArray;
            genericParams.push_back(paramObj);
        }
        nodeJson["genericTypeParameters"] = genericParams;

        // Serialize body
        if (methodNode->getBodyPtr()) {
            nodeJson["body"] = serializeNode(methodNode->getBodyPtr());
        } else {
            nodeJson["body"] = nullptr;
        }

        const auto& location = methodNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeConstructorNode(const ASTNode* node)
    {
        auto constructorNode = dynamic_cast<const nodes::classes::ConstructorNode*>(node);
        if (!constructorNode) return nullptr;

        json nodeJson;
        nodeJson["nodeType"] = "ConstructorNode";
        nodeJson["nodeTypeValue"] = static_cast<int>(NodeType::CONSTRUCTOR_NODE);

        // Serialize parameters
        json paramsArray = json::array();
        for (const auto& param : constructorNode->getParameters()) {
            json paramObj;
            paramObj["name"] = param.first;
            paramObj["type"] = static_cast<int>(param.second);
            paramsArray.push_back(paramObj);
        }
        nodeJson["parameters"] = paramsArray;

        // Serialize body
        if (constructorNode->getBodyPtr()) {
            nodeJson["body"] = serializeNode(constructorNode->getBodyPtr());
        } else {
            nodeJson["body"] = nullptr;
        }

        const auto& location = constructorNode->getLocation();
        nodeJson["line"] = location.getLine();
        nodeJson["column"] = location.getColumn();

        return nodeJson;
    }

    // Helper methods
    CompleteJSONSerializer::json CompleteJSONSerializer::serializeValueType(ValueType type)
    {
        return static_cast<int>(type);
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeBinaryOperator(uint8_t op)
    {
        return static_cast<int>(op);
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeUnaryOperator(uint8_t op)
    {
        return static_cast<int>(op);
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeGenericTypeParameters(const std::vector<std::string>& params)
    {
        json paramsArray = json::array();
        for (const auto& param : params) {
            paramsArray.push_back(param);
        }
        return paramsArray;
    }

    CompleteJSONSerializer::json CompleteJSONSerializer::serializeParameters(const std::vector<std::pair<std::string, ValueType>>& params)
    {
        json paramsArray = json::array();
        for (const auto& param : params) {
            json paramObj;
            paramObj["name"] = param.first;
            paramObj["type"] = static_cast<int>(param.second);
            paramsArray.push_back(paramObj);
        }
        return paramsArray;
    }

    // Node type utilities
    NodeType CompleteJSONSerializer::getNodeType(const ASTNode* node)
    {
        if (!node) return static_cast<NodeType>(0);

        // Use dynamic_cast to determine node type
        if (dynamic_cast<const nodes::expressions::IntegerNode*>(node)) return NodeType::INTEGER_NODE;
        if (dynamic_cast<const nodes::expressions::FloatNode*>(node)) return NodeType::FLOAT_NODE;
        if (dynamic_cast<const nodes::expressions::StringNode*>(node)) return NodeType::STRING_NODE;
        if (dynamic_cast<const nodes::expressions::BoolNode*>(node)) return NodeType::BOOL_NODE;
        if (dynamic_cast<const nodes::expressions::NullNode*>(node)) return NodeType::NULL_NODE;
        if (dynamic_cast<const nodes::expressions::VariableNode*>(node)) return NodeType::VARIABLE_NODE;
        if (dynamic_cast<const nodes::expressions::BinaryExpNode*>(node)) return NodeType::BINARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::UnaryExpNode*>(node)) return NodeType::UNARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::TernaryExpNode*>(node)) return NodeType::TERNARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::IndexAccessNode*>(node)) return NodeType::INDEX_ACCESS_NODE;
        if (dynamic_cast<const nodes::expressions::ArrayCreationNode*>(node)) return NodeType::ARRAY_CREATION_NODE;

        if (dynamic_cast<const nodes::statements::ProgramNode*>(node)) return NodeType::PROGRAM_NODE;
        if (dynamic_cast<const nodes::statements::BlockNode*>(node)) return NodeType::BLOCK_NODE;
        if (dynamic_cast<const nodes::statements::DeclarationNode*>(node)) return NodeType::DECLARATION_NODE;
        if (dynamic_cast<const nodes::statements::AssignmentNode*>(node)) return NodeType::ASSIGNMENT_NODE;
        if (dynamic_cast<const nodes::statements::MemberAssignmentNode*>(node)) return NodeType::MEMBER_ASSIGNMENT_NODE;
        if (dynamic_cast<const nodes::statements::IndexAssignmentNode*>(node)) return NodeType::INDEX_ASSIGNMENT_NODE;
        if (dynamic_cast<const nodes::statements::IfNode*>(node)) return NodeType::IF_NODE;
        if (dynamic_cast<const nodes::statements::WhileNode*>(node)) return NodeType::WHILE_NODE;
        if (dynamic_cast<const nodes::statements::DoWhileNode*>(node)) return NodeType::DO_WHILE_NODE;
        if (dynamic_cast<const nodes::statements::ForNode*>(node)) return NodeType::FOR_NODE;
        if (dynamic_cast<const nodes::statements::ForEachNode*>(node)) return NodeType::FOR_EACH_NODE;
        if (dynamic_cast<const nodes::statements::BreakNode*>(node)) return NodeType::BREAK_NODE;
        if (dynamic_cast<const nodes::statements::ContinueNode*>(node)) return NodeType::CONTINUE_NODE;
        if (dynamic_cast<const nodes::statements::SwitchNode*>(node)) return NodeType::SWITCH_NODE;
        if (dynamic_cast<const nodes::statements::CaseNode*>(node)) return NodeType::CASE_NODE;
        if (dynamic_cast<const nodes::statements::DefaultCaseNode*>(node)) return NodeType::DEFAULT_CASE_NODE;
        if (dynamic_cast<const nodes::statements::ImportNode*>(node)) return NodeType::IMPORT_NODE;

        if (dynamic_cast<const nodes::functions::FunctionNode*>(node)) return NodeType::FUNCTION_NODE;
        if (dynamic_cast<const nodes::functions::FunctionCallNode*>(node)) return NodeType::FUNCTION_CALL_NODE;
        if (dynamic_cast<const nodes::functions::ReturnNode*>(node)) return NodeType::RETURN_NODE;

        if (dynamic_cast<const nodes::classes::ClassNode*>(node)) return NodeType::CLASS_NODE;
        if (dynamic_cast<const nodes::classes::NewNode*>(node)) return NodeType::NEW_NODE;
        if (dynamic_cast<const nodes::classes::MemberAccessNode*>(node)) return NodeType::MEMBER_ACCESS_NODE;
        if (dynamic_cast<const nodes::classes::MethodCallNode*>(node)) return NodeType::METHOD_CALL_NODE;
        if (dynamic_cast<const nodes::classes::FieldNode*>(node)) return NodeType::FIELD_NODE;
        if (dynamic_cast<const nodes::classes::MethodNode*>(node)) return NodeType::METHOD_NODE;
        if (dynamic_cast<const nodes::classes::ConstructorNode*>(node)) return NodeType::CONSTRUCTOR_NODE;

        return static_cast<NodeType>(0); // Unknown type
    }

    std::string CompleteJSONSerializer::getNodeTypeName(NodeType type)
    {
        switch (type) {
            case NodeType::INTEGER_NODE: return "IntegerNode";
            case NodeType::FLOAT_NODE: return "FloatNode";
            case NodeType::STRING_NODE: return "StringNode";
            case NodeType::BOOL_NODE: return "BoolNode";
            case NodeType::NULL_NODE: return "NullNode";
            case NodeType::VARIABLE_NODE: return "VariableNode";
            case NodeType::BINARY_EXP_NODE: return "BinaryExpNode";
            case NodeType::UNARY_EXP_NODE: return "UnaryExpNode";
            case NodeType::TERNARY_EXP_NODE: return "TernaryExpNode";
            case NodeType::INDEX_ACCESS_NODE: return "IndexAccessNode";
            case NodeType::ARRAY_CREATION_NODE: return "ArrayCreationNode";

            case NodeType::PROGRAM_NODE: return "ProgramNode";
            case NodeType::BLOCK_NODE: return "BlockNode";
            case NodeType::DECLARATION_NODE: return "DeclarationNode";
            case NodeType::ASSIGNMENT_NODE: return "AssignmentNode";
            case NodeType::MEMBER_ASSIGNMENT_NODE: return "MemberAssignmentNode";
            case NodeType::INDEX_ASSIGNMENT_NODE: return "IndexAssignmentNode";
            case NodeType::IF_NODE: return "IfNode";
            case NodeType::WHILE_NODE: return "WhileNode";
            case NodeType::DO_WHILE_NODE: return "DoWhileNode";
            case NodeType::FOR_NODE: return "ForNode";
            case NodeType::FOR_EACH_NODE: return "ForEachNode";
            case NodeType::BREAK_NODE: return "BreakNode";
            case NodeType::CONTINUE_NODE: return "ContinueNode";
            case NodeType::SWITCH_NODE: return "SwitchNode";
            case NodeType::CASE_NODE: return "CaseNode";
            case NodeType::DEFAULT_CASE_NODE: return "DefaultCaseNode";
            case NodeType::IMPORT_NODE: return "ImportNode";

            case NodeType::FUNCTION_NODE: return "FunctionNode";
            case NodeType::FUNCTION_CALL_NODE: return "FunctionCallNode";
            case NodeType::RETURN_NODE: return "ReturnNode";

            case NodeType::CLASS_NODE: return "ClassNode";
            case NodeType::NEW_NODE: return "NewNode";
            case NodeType::MEMBER_ACCESS_NODE: return "MemberAccessNode";
            case NodeType::METHOD_CALL_NODE: return "MethodCallNode";
            case NodeType::FIELD_NODE: return "FieldNode";
            case NodeType::METHOD_NODE: return "MethodNode";
            case NodeType::CONSTRUCTOR_NODE: return "ConstructorNode";

            default: return "UnknownNode";
        }
    }

    // Import resolution implementation
    bool CompleteJSONSerializer::serializeToFileWithImports(const std::shared_ptr<ASTNode>& root, const std::string& filename, const std::string& baseDirectory)
    {
        this->baseDirectory = baseDirectory;
        processedImports.clear();
        loadedImports.clear();

        // First resolve all imports recursively
        resolveImportsRecursively(root.get());

        // Then serialize to file
        return serializeToFile(root, filename);
    }

    void CompleteJSONSerializer::resolveImportsRecursively(const ASTNode* root)
    {
        if (!root) return;

        // Check if this is an ImportNode
        if (auto importNode = dynamic_cast<const nodes::statements::ImportNode*>(root)) {
            std::string importPath = importNode->getFilePath();

            // Avoid circular imports
            if (processedImports.find(importPath) == processedImports.end()) {
                processedImports.insert(importPath);

                // Load the imported AST
                auto importedAST = loadImportedAST(importPath);
                if (importedAST) {
                    // Recursively resolve imports in the loaded AST
                    resolveImportsRecursively(importedAST.get());
                    // Keep the AST alive
                    loadedImports.push_back(std::move(importedAST));
                }
            }
            return;
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<const nodes::statements::ProgramNode*>(root)) {
            for (const auto& statement : programNode->getStatements()) {
                resolveImportsRecursively(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<const nodes::statements::BlockNode*>(root)) {
            for (const auto& statement : blockNode->getStatements()) {
                resolveImportsRecursively(statement.get());
            }
        }
        else if (auto classNode = dynamic_cast<const nodes::classes::ClassNode*>(root)) {
            for (const auto& field : classNode->getFields()) {
                resolveImportsRecursively(field.get());
            }
            for (const auto& constructor : classNode->getConstructors()) {
                resolveImportsRecursively(constructor.get());
            }
            for (const auto& method : classNode->getMethods()) {
                resolveImportsRecursively(method.get());
            }
        }
        // Add other node types that may contain children as needed
    }

    std::unique_ptr<ASTNode> CompleteJSONSerializer::loadImportedAST(const std::string& importPath)
    {
        try {
            std::string resolvedPath = resolveImportPath(importPath);

            // Check if file exists
            if (!std::filesystem::exists(resolvedPath)) {
                std::cerr << "Import file not found: " << resolvedPath << std::endl;
                return nullptr;
            }

            // Use the same parser/lexer setup as ImportManager
            lexer::Lexer lexer(resolvedPath);
            parser::Parser parser(lexer, nullptr);  // Pass nullptr for ImportManager to avoid circular dependencies

            auto ast = parser.parseProgram();
            return ast;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load imported file: " << importPath << " - " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::string CompleteJSONSerializer::resolveImportPath(const std::string& importPath)
    {
        // Handle relative paths
        if (importPath.substr(0, 3) == "../" || importPath.substr(0, 2) == "./") {
            std::filesystem::path basePath(baseDirectory);
            std::filesystem::path fullPath = basePath / importPath;
            return std::filesystem::canonical(fullPath).string();
        }

        // Handle absolute paths or paths relative to base directory
        std::filesystem::path basePath(baseDirectory);
        std::filesystem::path fullPath = basePath / importPath;

        if (std::filesystem::exists(fullPath)) {
            return fullPath.string();
        }

        // Fallback to original path
        return importPath;
    }
}