#include "CompleteJSONDeserializer.hpp"
#include <fstream>
#include <iostream>

// Include all essential nodes for complete deserialization
#include "../nodes/statements/ProgramNode.hpp"
#include "../nodes/statements/ImportNode.hpp"
#include "../nodes/statements/BlockNode.hpp"
#include "../nodes/statements/DeclarationNode.hpp"
#include "../nodes/statements/AssignmentNode.hpp"
#include "../nodes/statements/IndexAssignmentNode.hpp"
#include "../nodes/statements/MemberAssignmentNode.hpp"
#include "../nodes/functions/ReturnNode.hpp"
#include "../nodes/statements/IfNode.hpp"
#include "../nodes/statements/WhileNode.hpp"
#include "../nodes/statements/ForNode.hpp"
#include "../nodes/statements/ForEachNode.hpp"
#include "../nodes/classes/ClassNode.hpp"
#include "../nodes/classes/FieldNode.hpp"
#include "../nodes/classes/MethodNode.hpp"
#include "../nodes/classes/ConstructorNode.hpp"
#include "../nodes/classes/NewNode.hpp"
#include "../nodes/classes/MemberAccessNode.hpp"
#include "../nodes/classes/MethodCallNode.hpp"
#include "../nodes/functions/FunctionNode.hpp"
#include "../nodes/functions/FunctionCallNode.hpp"
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
#include "../GenericTypeParameter.hpp"
#include "../GenericType.hpp"
#include "../../token/TokenType.hpp"

namespace ast::serialization
{
    CompleteJSONDeserializer::CompleteJSONDeserializer()
    {
    }

    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserialize(const std::string& jsonString)
    {
        try
        {
            json jsonData = json::parse(jsonString);
            return deserializeNode(jsonData);
        }
        catch (const json::exception& e)
        {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeFromFile(const std::string& filename)
    {
        try
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                std::cerr << "Failed to open JSON file: " << filename << std::endl;
                return nullptr;
            }
            json jsonData;
            file >> jsonData;

            auto result = deserializeNode(jsonData);
            return result;
        }
        catch (const json::exception& e)
        {
            std::cerr << "JSON parsing error in file " << filename << ": " << e.what() << std::endl;
            return nullptr;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error reading file " << filename << ": " << e.what() << std::endl;
            return nullptr;
        }
    }

    void CompleteJSONDeserializer::clearImportCache()
    {
        importCache.clear();
    }

    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeNode(const json& nodeJson)
    {
        auto uniqueNode = deserializeNodeUnique(nodeJson);
        if (uniqueNode)
        {
            return std::shared_ptr<ASTNode>(uniqueNode.release());
        }
        return nullptr;
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique(const json& nodeJson)
    {
        try {
            if (nodeJson.is_null())
                return nullptr;

            NodeType nodeType = getNodeTypeFromJson(nodeJson);

            switch (nodeType)
        {
        // Program and Import nodes
        case NodeType::PROGRAM_NODE:
            return deserializeNodeUnique_Program(nodeJson);
        case NodeType::IMPORT_NODE:
            return deserializeNodeUnique_Import(nodeJson);

        // Expression nodes
        case NodeType::INTEGER_NODE:
            return deserializeNodeUnique_Integer(nodeJson);
        case NodeType::FLOAT_NODE:
            return deserializeNodeUnique_Float(nodeJson);
        case NodeType::STRING_NODE:
            return deserializeNodeUnique_String(nodeJson);
        case NodeType::BOOL_NODE:
            return deserializeNodeUnique_Bool(nodeJson);
        case NodeType::NULL_NODE:
            return deserializeNodeUnique_Null(nodeJson);
        case NodeType::VARIABLE_NODE:
            return deserializeNodeUnique_Variable(nodeJson);
        case NodeType::BINARY_EXP_NODE:
            return deserializeNodeUnique_BinaryExp(nodeJson);
        case NodeType::UNARY_EXP_NODE:
            return deserializeNodeUnique_UnaryExp(nodeJson);
        case NodeType::TERNARY_EXP_NODE:
            return deserializeNodeUnique_TernaryExp(nodeJson);
        case NodeType::INDEX_ACCESS_NODE:
            return deserializeNodeUnique_IndexAccess(nodeJson);
        case NodeType::ARRAY_CREATION_NODE:
            return deserializeNodeUnique_ArrayCreation(nodeJson);

        // Statement nodes
        case NodeType::BLOCK_NODE:
            return deserializeNodeUnique_Block(nodeJson);
        case NodeType::DECLARATION_NODE:
            return deserializeNodeUnique_Declaration(nodeJson);
        case NodeType::ASSIGNMENT_NODE:
            return deserializeNodeUnique_Assignment(nodeJson);
        case NodeType::INDEX_ASSIGNMENT_NODE:
            return deserializeNodeUnique_IndexAssignment(nodeJson);
        case NodeType::MEMBER_ASSIGNMENT_NODE:
            return deserializeNodeUnique_MemberAssignment(nodeJson);
        case NodeType::RETURN_NODE:
            return deserializeNodeUnique_Return(nodeJson);
        case NodeType::IF_NODE:
            return deserializeNodeUnique_If(nodeJson);
        case NodeType::WHILE_NODE:
            return deserializeNodeUnique_While(nodeJson);
        case NodeType::FOR_NODE:
            return deserializeNodeUnique_For(nodeJson);
        case NodeType::FOR_EACH_NODE:
            return deserializeNodeUnique_ForEach(nodeJson);

        // Function nodes
        case NodeType::FUNCTION_NODE:
            return deserializeNodeUnique_Function(nodeJson);
        case NodeType::FUNCTION_CALL_NODE:
            return deserializeNodeUnique_FunctionCall(nodeJson);

        // Class nodes
        case NodeType::CLASS_NODE:
            return deserializeNodeUnique_Class(nodeJson);
        case NodeType::FIELD_NODE:
            return deserializeNodeUnique_Field(nodeJson);
        case NodeType::METHOD_NODE:
            return deserializeNodeUnique_Method(nodeJson);
        case NodeType::CONSTRUCTOR_NODE:
            return deserializeNodeUnique_Constructor(nodeJson);
        case NodeType::NEW_NODE:
            return deserializeNodeUnique_New(nodeJson);
        case NodeType::MEMBER_ACCESS_NODE:
            return deserializeNodeUnique_MemberAccess(nodeJson);
        case NodeType::METHOD_CALL_NODE:
            return deserializeNodeUnique_MethodCall(nodeJson);

        default:
            std::cerr << "Unhandled node type in JSON deserializer: " << static_cast<int>(nodeType)
                      << " (" << (nodeJson.contains("nodeType") ? nodeJson["nodeType"].get<std::string>() : "unknown") << ")" << std::endl;
            return nullptr;
        }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize node: " << e.what() << std::endl;
            return nullptr;
        }
    }

    NodeType CompleteJSONDeserializer::getNodeTypeFromJson(const json& nodeJson)
    {
        if (nodeJson.contains("nodeTypeValue"))
        {
            return static_cast<NodeType>(nodeJson["nodeTypeValue"].get<int>());
        }
        throw std::runtime_error("JSON node missing nodeTypeValue field");
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Program(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);
        auto programNode = std::make_unique<nodes::statements::ProgramNode>(location);

        if (nodeJson.contains("statements") && nodeJson["statements"].is_array())
        {
            for (const auto& stmtJson : nodeJson["statements"])
            {
                auto stmt = deserializeNodeUnique(stmtJson);
                if (stmt)
                {
                    programNode->addStatement(std::move(stmt));
                }
            }
        }

        return std::move(programNode);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Import(const json& nodeJson)
    {
        std::string path;
        if (nodeJson.contains("path")) {
            path = nodeJson["path"].get<std::string>();
        } else if (nodeJson.contains("filePath")) {
            path = nodeJson["filePath"].get<std::string>();
        } else {
            path = "";
        }

        auto location = deserializeSourceLocation(nodeJson);

        auto importNode = std::make_unique<nodes::statements::ImportNode>(path, location);

        // Reconstruct imported AST if it was resolved during serialization
        if (nodeJson.contains("isResolved") && nodeJson["isResolved"].get<bool>() &&
            nodeJson.contains("importedAST") && !nodeJson["importedAST"].is_null())
        {
            auto importedAST = deserializeNode(nodeJson["importedAST"]); // Use shared_ptr version for caching
            if (importedAST)
            {
                importNode->setImportedAST(importedAST.get());
                // Cache the imported AST to keep it alive
                importCache[path] = importedAST;
            }
        }

        return std::move(importNode);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Class(const json& nodeJson)
    {
        std::string className = nodeJson["className"].get<std::string>();
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize generic parameters
        std::vector<GenericTypeParameter> genericParams;
        if (nodeJson.contains("genericParameters") && nodeJson["genericParameters"].is_array())
        {
            for (const auto& paramJson : nodeJson["genericParameters"])
            {
                if (paramJson.is_string())
                {
                    genericParams.emplace_back(paramJson.get<std::string>(), location);
                }
            }
        }

        // Create class node with generic parameters
        auto classNode = std::make_unique<nodes::classes::ClassNode>(className, genericParams, location);

        // Deserialize fields
        if (nodeJson.contains("fields") && nodeJson["fields"].is_array())
        {
            for (const auto& fieldJson : nodeJson["fields"])
            {
                auto field = deserializeNodeUnique(fieldJson);
                if (field)
                {
                    classNode->addField(std::move(field));
                }
            }
        }

        // Deserialize methods
        if (nodeJson.contains("methods") && nodeJson["methods"].is_array())
        {
            for (const auto& methodJson : nodeJson["methods"])
            {
                auto method = deserializeNodeUnique(methodJson);
                if (method)
                {
                    classNode->addMethod(std::move(method));
                }
            }
        }

        // Deserialize constructors
        if (nodeJson.contains("constructors") && nodeJson["constructors"].is_array())
        {
            for (const auto& constructorJson : nodeJson["constructors"])
            {
                auto constructor = deserializeNodeUnique(constructorJson);
                if (constructor)
                {
                    classNode->addConstructor(std::move(constructor));
                }
            }
        }

        return std::move(classNode);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Function(const json& nodeJson)
    {
        try {
            std::string functionName = nodeJson.contains("name") ? nodeJson["name"].get<std::string>() : "";
            value::ValueType returnType = nodeJson.contains("returnType") ? static_cast<value::ValueType>(nodeJson["returnType"].get<int>()) : value::ValueType::VOID;
            auto params = deserializeParameters(nodeJson.contains("parameters") ? nodeJson["parameters"] : json::array());
            auto location = deserializeSourceLocation(nodeJson);

            std::shared_ptr<ASTNode> body = nullptr;
            if (nodeJson.contains("body") && !nodeJson["body"].is_null())
            {
                auto uniqueBody = deserializeNodeUnique(nodeJson["body"]);
                if (uniqueBody)
                {
                    body = std::shared_ptr<ASTNode>(uniqueBody.release());
                }
            }

            return std::make_unique<nodes::functions::FunctionNode>(functionName, returnType, params, body, location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize FunctionNode: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_FunctionCall(const json& nodeJson)
    {
        try {
            std::string functionName = nodeJson.contains("functionName") ? nodeJson["functionName"].get<std::string>() : "";
            auto location = deserializeSourceLocation(nodeJson);

            // Deserialize arguments
            std::vector<std::unique_ptr<ASTNode>> args;
            if (nodeJson.contains("arguments") && nodeJson["arguments"].is_array()) {
                for (const auto& argJson : nodeJson["arguments"]) {
                    auto arg = deserializeNodeUnique(argJson);
                    if (arg) {
                        args.push_back(std::move(arg));
                    }
                }
            }

            return std::make_unique<nodes::functions::FunctionCallNode>(functionName, std::move(args), location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize FunctionCallNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    errors::SourceLocation CompleteJSONDeserializer::deserializeSourceLocation(const json& nodeJson)
    {
        try {
            std::string filename = nodeJson.contains("filename") ? nodeJson["filename"].get<std::string>() : "<deserialized>";
            int line = nodeJson.contains("line") ? nodeJson["line"].get<int>() : 1;
            int column = nodeJson.contains("column") ? nodeJson["column"].get<int>() : 1;
            return errors::SourceLocation(filename, line, column);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize SourceLocation: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON fields: ";
            for (auto& [key, value] : nodeJson.items()) {
                std::cerr << key << " ";
            }
            std::cerr << std::endl;
            return errors::SourceLocation("<error>", 1, 1);
        }
    }

    // Helper method implementations
    ValueType CompleteJSONDeserializer::deserializeValueType(int typeValue) { return static_cast<ValueType>(typeValue); }
    uint8_t CompleteJSONDeserializer::deserializeBinaryOperator(int opValue) { return static_cast<uint8_t>(opValue); }
    uint8_t CompleteJSONDeserializer::deserializeUnaryOperator(int opValue) { return static_cast<uint8_t>(opValue); }

    std::vector<std::string> CompleteJSONDeserializer::deserializeGenericTypeParameters(const json& paramsJson)
    {
        std::vector<std::string> params;
        if (paramsJson.is_array()) {
            for (const auto& param : paramsJson) {
                if (param.is_string()) {
                    params.push_back(param.get<std::string>());
                }
            }
        }
        return params;
    }

    std::vector<std::pair<std::string, value::ValueType>> CompleteJSONDeserializer::deserializeParameters(const json& paramsJson)
    {
        std::vector<std::pair<std::string, value::ValueType>> params;
        if (paramsJson.is_array()) {
            for (const auto& param : paramsJson) {
                if (param.contains("name") && param.contains("type")) {
                    std::string name = param["name"].get<std::string>();
                    value::ValueType type = static_cast<value::ValueType>(param["type"].get<int>());
                    params.emplace_back(name, type);
                }
            }
        }
        return params;
    }

    // Expression node implementations
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Integer(const json& nodeJson)
    {
        int value = nodeJson["value"].get<int>();
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::expressions::IntegerNode>(value, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Float(const json& nodeJson)
    {
        double value = nodeJson["value"].get<double>();
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::expressions::FloatNode>(value, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_String(const json& nodeJson)
    {
        std::string value = nodeJson["value"].get<std::string>();
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::expressions::StringNode>(value, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Bool(const json& nodeJson)
    {
        bool value = nodeJson["value"].get<bool>();
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::expressions::BoolNode>(value, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Null(const json& nodeJson)
    {
        try {

            auto location = deserializeSourceLocation(nodeJson);

            return std::make_unique<nodes::expressions::NullNode>(location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize NullNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Variable(const json& nodeJson)
    {
        try {

            if (!nodeJson.contains("name")) {
                std::cerr << "[ERROR] VariableNode: Missing 'name' field!" << std::endl;
                std::cerr << "[DEBUG] Available fields: ";
                for (auto& [key, value] : nodeJson.items()) {
                    std::cerr << key << " ";
                }
                std::cerr << std::endl;
                return nullptr;
            }

            std::string name = nodeJson["name"].get<std::string>();

            auto location = deserializeSourceLocation(nodeJson);

            return std::make_unique<nodes::expressions::VariableNode>(name, location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize VariableNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_BinaryExp(const json& nodeJson)
    {
        try {


            if (!nodeJson.contains("left") || !nodeJson.contains("right") || !nodeJson.contains("operator")) {
                std::cerr << "[ERROR] BinaryExpNode: Missing required fields!" << std::endl;
                return nullptr;
            }

            auto left = deserializeNodeUnique(nodeJson["left"]);
            if (!left) {
                std::cerr << "[ERROR] BinaryExpNode: Failed to deserialize left operand" << std::endl;
                return nullptr;
            }

            auto right = deserializeNodeUnique(nodeJson["right"]);
            if (!right) {
                std::cerr << "[ERROR] BinaryExpNode: Failed to deserialize right operand" << std::endl;
                return nullptr;
            }

            int opValue = nodeJson["operator"].get<int>();
            token::TokenType op = static_cast<token::TokenType>(opValue);
            auto location = deserializeSourceLocation(nodeJson);

            return std::make_unique<nodes::expressions::BinaryExpNode>(std::move(left), op, std::move(right), location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize BinaryExpNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_UnaryExp(const json& nodeJson)
    {
        auto operand = deserializeNodeUnique(nodeJson["operand"]);
        token::TokenType op = static_cast<token::TokenType>(nodeJson["operator"].get<int>());
        auto location = deserializeSourceLocation(nodeJson);
        // Default to PREFIX position for now
        return std::make_unique<nodes::expressions::UnaryExpNode>(op, std::move(operand), nodes::expressions::UnaryPosition::PREFIX, location);
    }

    // Statement node implementations
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Block(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);
        auto blockNode = std::make_unique<nodes::statements::BlockNode>(location);

        if (nodeJson.contains("statements") && nodeJson["statements"].is_array())
        {
            for (const auto& stmtJson : nodeJson["statements"])
            {
                auto stmt = deserializeNodeUnique(stmtJson);
                if (stmt)
                {
                    blockNode->addStatement(std::move(stmt));
                }
            }
        }
        return std::move(blockNode);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Declaration(const json& nodeJson)
    {
        std::string name = nodeJson["variableName"].get<std::string>();
        value::ValueType type = static_cast<value::ValueType>(nodeJson["type"].get<int>());
        auto location = deserializeSourceLocation(nodeJson);

        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (nodeJson.contains("initialValue") && !nodeJson["initialValue"].is_null())
        {
            initialValue = deserializeNodeUnique(nodeJson["initialValue"]);
        }

        bool isFinal = nodeJson.contains("isFinal") ? nodeJson["isFinal"].get<bool>() : false;
        bool isConst = nodeJson.contains("isConst") ? nodeJson["isConst"].get<bool>() : false;
        return std::make_unique<nodes::statements::DeclarationNode>(name, type, std::move(initialValue), isFinal, isConst, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Assignment(const json& nodeJson)
    {
        std::string varName = nodeJson["variableName"].get<std::string>();
        auto value = deserializeNodeUnique(nodeJson["value"]);
        value::ValueType type = nodeJson.contains("variableType") ? static_cast<value::ValueType>(nodeJson["variableType"].get<int>()) : value::ValueType::VOID;
        std::string className = nodeJson.contains("className") ? nodeJson["className"].get<std::string>() : "";
        bool isFinal = nodeJson.contains("isFinal") ? nodeJson["isFinal"].get<bool>() : false;
        bool isStatic = nodeJson.contains("isStatic") ? nodeJson["isStatic"].get<bool>() : false;
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::statements::AssignmentNode>(varName, std::move(value), type, className, isFinal, isStatic, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Return(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);
        std::unique_ptr<ASTNode> value = nullptr;
        if (nodeJson.contains("value") && !nodeJson["value"].is_null())
        {
            value = deserializeNodeUnique(nodeJson["value"]);
        }
        return std::make_unique<nodes::functions::ReturnNode>(std::move(value), location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_If(const json& nodeJson)
    {
        try {

            auto condition = deserializeNodeUnique(nodeJson["condition"]);
            if (!condition) {
                std::cerr << "[ERROR] IfNode: Failed to deserialize condition" << std::endl;
                return nullptr;
            }

            auto thenBody = deserializeNodeUnique(nodeJson["thenStatement"]);
            if (!thenBody) {
                std::cerr << "[ERROR] IfNode: Failed to deserialize then statement" << std::endl;
                return nullptr;
            }

            auto location = deserializeSourceLocation(nodeJson);

            std::unique_ptr<ASTNode> elseBody = nullptr;
            if (nodeJson.contains("elseStatement") && !nodeJson["elseStatement"].is_null())
            {
                elseBody = deserializeNodeUnique(nodeJson["elseStatement"]);
            }

            return std::make_unique<nodes::statements::IfNode>(std::move(condition), std::move(thenBody), std::move(elseBody), location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize IfNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_While(const json& nodeJson)
    {
        auto condition = deserializeNodeUnique(nodeJson["condition"]);
        auto body = deserializeNodeUnique(nodeJson["body"]);
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::statements::WhileNode>(std::move(condition), std::move(body), location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_For(const json& nodeJson)
    {
        try {


            auto initializer = deserializeNodeUnique(nodeJson["initialization"]);

            auto condition = deserializeNodeUnique(nodeJson["condition"]);

            auto increment = deserializeNodeUnique(nodeJson["update"]);

            auto body = deserializeNodeUnique(nodeJson["body"]);

            auto location = deserializeSourceLocation(nodeJson);

            return std::make_unique<nodes::statements::ForNode>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] ForNode deserialization failed: " << e.what() << std::endl;
            throw;
        }
    }

    // Field node implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Field(const json& nodeJson)
    {
        std::string name = nodeJson["name"].get<std::string>();
        value::ValueType type = static_cast<value::ValueType>(nodeJson["type"].get<int>());
        bool isStatic = nodeJson.contains("isStatic") ? nodeJson["isStatic"].get<bool>() : false;
        bool isFinal = nodeJson.contains("isFinal") ? nodeJson["isFinal"].get<bool>() : false;
        auto location = deserializeSourceLocation(nodeJson);

        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (nodeJson.contains("hasInitialValue") && nodeJson["hasInitialValue"].get<bool>() &&
            nodeJson.contains("initialValue") && !nodeJson["initialValue"].is_null())
        {
            initialValue = deserializeNodeUnique(nodeJson["initialValue"]);
        }

        return std::make_unique<nodes::classes::FieldNode>(name, type, std::move(initialValue), isStatic, isFinal, location);
    }

    // Method node implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Method(const json& nodeJson)
    {
        try {

            std::string name = nodeJson.contains("name") ? nodeJson["name"].get<std::string>() : "";

            value::ValueType returnType = nodeJson.contains("returnType") ? static_cast<value::ValueType>(nodeJson["returnType"].get<int>()) : value::ValueType::VOID;

            auto params = deserializeParameters(nodeJson.contains("parameters") ? nodeJson["parameters"] : json::array());

            bool isStatic = nodeJson.contains("isStatic") ? nodeJson["isStatic"].get<bool>() : false;

            auto location = deserializeSourceLocation(nodeJson);

            std::unique_ptr<ASTNode> body = nullptr;
            if (nodeJson.contains("body") && !nodeJson["body"].is_null())
            {
                body = deserializeNodeUnique(nodeJson["body"]);
            }

            return std::make_unique<nodes::classes::MethodNode>(name, returnType, params, std::move(body), isStatic, location);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to deserialize MethodNode: " << e.what() << std::endl;
            std::cerr << "[DEBUG] JSON content: " << nodeJson.dump(2) << std::endl;
            return nullptr;
        }
    }

    // Constructor node implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_Constructor(const json& nodeJson)
    {
        auto params = deserializeParameters(nodeJson.contains("parameters") ? nodeJson["parameters"] : json::array());
        auto location = deserializeSourceLocation(nodeJson);

        std::unique_ptr<ASTNode> body = nullptr;
        if (nodeJson.contains("body") && !nodeJson["body"].is_null())
        {
            body = deserializeNodeUnique(nodeJson["body"]);
        }

        return std::make_unique<nodes::classes::ConstructorNode>(params, std::move(body), location);
    }

    // Remaining class node implementations - simplified versions for now
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_New(const json& nodeJson)
    {
        std::string className = nodeJson.contains("className") ? nodeJson["className"].get<std::string>() : "";
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize constructor arguments
        std::vector<std::unique_ptr<ASTNode>> args;
        if (nodeJson.contains("arguments") && nodeJson["arguments"].is_array()) {
            for (const auto& arg : nodeJson["arguments"]) {
                args.push_back(deserializeNodeUnique(arg));
            }
        }

        return std::make_unique<nodes::classes::NewNode>(className, std::move(args), location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_MemberAccess(const json& nodeJson)
    {
        auto object = deserializeNodeUnique(nodeJson["object"]);
        std::string memberName = nodeJson["memberName"].get<std::string>();
        bool isStatic = nodeJson.contains("isStaticAccess") ? nodeJson["isStaticAccess"].get<bool>() : false;
        auto location = deserializeSourceLocation(nodeJson);
        return std::make_unique<nodes::classes::MemberAccessNode>(std::move(object), memberName, isStatic, location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_MethodCall(const json& nodeJson)
    {
        auto object = deserializeNodeUnique(nodeJson["object"]);
        std::string methodName = nodeJson["methodName"].get<std::string>();
        bool isStatic = nodeJson.contains("isStaticCall") ? nodeJson["isStaticCall"].get<bool>() : false;
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize arguments
        std::vector<std::unique_ptr<ASTNode>> args;
        if (nodeJson.contains("arguments") && nodeJson["arguments"].is_array()) {
            for (const auto& arg : nodeJson["arguments"]) {
                args.push_back(deserializeNodeUnique(arg));
            }
        }

        // Deserialize generic type arguments
        std::vector<std::string> genericArgs;
        if (nodeJson.contains("genericTypeArguments") && nodeJson["genericTypeArguments"].is_array()) {
            for (const auto& genericArg : nodeJson["genericTypeArguments"]) {
                genericArgs.push_back(genericArg.get<std::string>());
            }
        }

        return std::make_unique<nodes::classes::MethodCallNode>(std::move(object), methodName, std::move(args), isStatic, genericArgs, location);
    }

    // IndexAssignmentNode implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_IndexAssignment(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize object, index, and value
        auto object = deserializeNodeUnique(nodeJson["object"]);
        auto index = deserializeNodeUnique(nodeJson["index"]);
        auto value = deserializeNodeUnique(nodeJson["value"]);

        return std::make_unique<nodes::statements::IndexAssignmentNode>(
            std::move(object), std::move(index), std::move(value), location);
    }

    // MemberAssignmentNode implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_MemberAssignment(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize object, member name, and value
        auto object = deserializeNodeUnique(nodeJson["object"]);
        std::string memberName = nodeJson["memberName"].get<std::string>();
        auto value = deserializeNodeUnique(nodeJson["value"]);

        return std::make_unique<nodes::statements::MemberAssignmentNode>(
            std::move(object), memberName, std::move(value), location);
    }

    // IndexAccessNode implementation
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_IndexAccess(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize collection and index
        auto collection = deserializeNodeUnique(nodeJson["collection"]);
        auto index = deserializeNodeUnique(nodeJson["index"]);

        return std::make_unique<nodes::expressions::IndexAccessNode>(
            std::move(collection), std::move(index), location);
    }

    // Additional node implementations
    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_TernaryExp(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);
        auto condition = deserializeNodeUnique(nodeJson["condition"]);
        auto trueExpression = deserializeNodeUnique(nodeJson["trueExpression"]);
        auto falseExpression = deserializeNodeUnique(nodeJson["falseExpression"]);
        return std::make_unique<nodes::expressions::TernaryExpNode>(
            std::move(condition), std::move(trueExpression), std::move(falseExpression), location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_ArrayCreation(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);

        // Deserialize element type - simplified for now
        parser::TypeInfo elementTypeInfo;
        if (nodeJson.contains("elementType")) {
            elementTypeInfo.baseType = static_cast<value::ValueType>(nodeJson["elementType"].get<int>());
        }

        // Deserialize size expressions
        std::vector<std::unique_ptr<ASTNode>> sizeExprs;
        if (nodeJson.contains("sizeExpressions") && nodeJson["sizeExpressions"].is_array()) {
            for (const auto& sizeJson : nodeJson["sizeExpressions"]) {
                sizeExprs.push_back(deserializeNodeUnique(sizeJson));
            }
        } else if (nodeJson.contains("sizeExpression")) {
            sizeExprs.push_back(deserializeNodeUnique(nodeJson["sizeExpression"]));
        }

        return std::make_unique<nodes::expressions::ArrayCreationNode>(elementTypeInfo, std::move(sizeExprs), location);
    }

    std::unique_ptr<ASTNode> CompleteJSONDeserializer::deserializeNodeUnique_ForEach(const json& nodeJson)
    {
        auto location = deserializeSourceLocation(nodeJson);
        std::string variableName = nodeJson["variableName"].get<std::string>();

        // Deserialize variable type
        parser::TypeInfo variableTypeInfo;
        if (nodeJson.contains("variableType")) {
            variableTypeInfo.baseType = static_cast<value::ValueType>(nodeJson["variableType"].get<int>());
        }

        auto collection = deserializeNodeUnique(nodeJson["collection"]);
        auto body = deserializeNodeUnique(nodeJson["body"]);

        return std::make_unique<nodes::statements::ForEachNode>(
            variableName, variableTypeInfo, std::move(collection), std::move(body), location);
    }

    // Legacy shared_ptr methods that delegate to the unique_ptr versions
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeIntegerNode(const json& nodeJson) { return deserializeNodeUnique_Integer(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeFloatNode(const json& nodeJson) { return deserializeNodeUnique_Float(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeStringNode(const json& nodeJson) { return deserializeNodeUnique_String(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeBoolNode(const json& nodeJson) { return deserializeNodeUnique_Bool(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeNullNode(const json& nodeJson) { return deserializeNodeUnique_Null(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeVariableNode(const json& nodeJson) { return deserializeNodeUnique_Variable(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeBinaryExpNode(const json& nodeJson) { return deserializeNodeUnique_BinaryExp(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeUnaryExpNode(const json& nodeJson) { return deserializeNodeUnique_UnaryExp(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeTernaryExpNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeIndexAccessNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeArrayCreationNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeProgramNode(const json& nodeJson) { return deserializeNodeUnique_Program(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeBlockNode(const json& nodeJson) { return deserializeNodeUnique_Block(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeDeclarationNode(const json& nodeJson) { return deserializeNodeUnique_Declaration(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeAssignmentNode(const json& nodeJson) { return deserializeNodeUnique_Assignment(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeMemberAssignmentNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeIndexAssignmentNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeReturnNode(const json& nodeJson) { return deserializeNodeUnique_Return(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeImportNode(const json& nodeJson) { return deserializeNodeUnique_Import(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeIfNode(const json& nodeJson) { return deserializeNodeUnique_If(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeWhileNode(const json& nodeJson) { return deserializeNodeUnique_While(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeDoWhileNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeForNode(const json& nodeJson) { return deserializeNodeUnique_For(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeForEachNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeBreakNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeContinueNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeSwitchNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeCaseNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeDefaultCaseNode(const json& nodeJson) { return nullptr; }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeFunctionNode(const json& nodeJson) { return deserializeNodeUnique_Function(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeFunctionCallNode(const json& nodeJson) { return deserializeNodeUnique_FunctionCall(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeClassNode(const json& nodeJson) { return deserializeNodeUnique_Class(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeNewNode(const json& nodeJson) { return deserializeNodeUnique_New(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeMemberAccessNode(const json& nodeJson) {
        // Deserialize the object being accessed
        std::shared_ptr<ASTNode> object = nullptr;
        if (nodeJson.contains("object") && !nodeJson["object"].is_null()) {
            object = deserializeNodeUnique(nodeJson["object"]);
        }

        // Extract member name and static access flag
        std::string memberName = nodeJson["memberName"];
        bool isStaticAccess = nodeJson["isStaticAccess"];

        // Extract location information
        auto location = deserializeSourceLocation(nodeJson);

        return std::make_shared<nodes::classes::MemberAccessNode>(object, memberName, isStaticAccess, location);
    }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeMethodCallNode(const json& nodeJson) { return deserializeNodeUnique_MethodCall(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeFieldNode(const json& nodeJson) { return deserializeNodeUnique_Field(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeMethodNode(const json& nodeJson) { return deserializeNodeUnique_Method(nodeJson); }
    std::shared_ptr<ASTNode> CompleteJSONDeserializer::deserializeConstructorNode(const json& nodeJson) { return deserializeNodeUnique_Constructor(nodeJson); }
}