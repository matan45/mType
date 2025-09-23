#pragma once
#include "../ASTNode.hpp"
#include "SerializationFormat.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <json.hpp>

namespace ast::serialization
{
    /**
     * @brief Complete JSON-based AST deserializer
     * Reconstructs AST from JSON serialized by CompleteJSONSerializer
     */
    class CompleteJSONDeserializer
    {
    private:
        // JSON library using nlohmann/json
        using json = nlohmann::json;

        // Cache for reconstructed import ASTs to avoid duplicate parsing
        std::unordered_map<std::string, std::shared_ptr<ASTNode>> importCache;

        // Core deserialization
        std::shared_ptr<ASTNode> deserializeNode(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique(const json& nodeJson);
        NodeType getNodeTypeFromJson(const json& nodeJson);

        // Expression node deserialization
        std::shared_ptr<ASTNode> deserializeIntegerNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeFloatNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeStringNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeBoolNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeNullNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeVariableNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeBinaryExpNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeUnaryExpNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeTernaryExpNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeIndexAccessNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeArrayCreationNode(const json& nodeJson);

        // Statement node deserialization
        std::shared_ptr<ASTNode> deserializeProgramNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeBlockNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeDeclarationNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeAssignmentNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeMemberAssignmentNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeIndexAssignmentNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeReturnNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeImportNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeIfNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeWhileNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeDoWhileNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeForNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeForEachNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeBreakNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeContinueNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeSwitchNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeCaseNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeDefaultCaseNode(const json& nodeJson);

        // Function node deserialization
        std::shared_ptr<ASTNode> deserializeFunctionNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeFunctionCallNode(const json& nodeJson);

        // Class node deserialization
        std::shared_ptr<ASTNode> deserializeClassNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeNewNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeMemberAccessNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeMethodCallNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeFieldNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeMethodNode(const json& nodeJson);
        std::shared_ptr<ASTNode> deserializeConstructorNode(const json& nodeJson);

        // Helper methods
        ValueType deserializeValueType(int typeValue);
        uint8_t deserializeBinaryOperator(int opValue);
        uint8_t deserializeUnaryOperator(int opValue);
        std::vector<std::string> deserializeGenericTypeParameters(const json& paramsJson);
        std::vector<std::pair<std::string, value::ValueType>> deserializeParameters(const json& paramsJson);
        errors::SourceLocation deserializeSourceLocation(const json& nodeJson);

        // Private unique_ptr helper methods - Program and Import
        std::unique_ptr<ASTNode> deserializeNodeUnique_Program(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Import(const json& nodeJson);

        // Expression nodes
        std::unique_ptr<ASTNode> deserializeNodeUnique_Integer(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Float(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_String(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Bool(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Null(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Variable(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_BinaryExp(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_UnaryExp(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_TernaryExp(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_IndexAccess(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_ArrayCreation(const json& nodeJson);

        // Statement nodes
        std::unique_ptr<ASTNode> deserializeNodeUnique_Block(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Declaration(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Assignment(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_IndexAssignment(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_MemberAssignment(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Return(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_If(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_While(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_For(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_ForEach(const json& nodeJson);

        // Function nodes
        std::unique_ptr<ASTNode> deserializeNodeUnique_Function(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_FunctionCall(const json& nodeJson);

        // Class nodes
        std::unique_ptr<ASTNode> deserializeNodeUnique_Class(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Field(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Method(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_Constructor(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_New(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_MemberAccess(const json& nodeJson);
        std::unique_ptr<ASTNode> deserializeNodeUnique_MethodCall(const json& nodeJson);

    public:
        CompleteJSONDeserializer();
        ~CompleteJSONDeserializer() = default;

        /**
         * @brief Deserialize AST from JSON string
         */
        std::shared_ptr<ASTNode> deserialize(const std::string& jsonString);

        /**
         * @brief Deserialize AST from JSON file
         */
        std::shared_ptr<ASTNode> deserializeFromFile(const std::string& filename);

        /**
         * @brief Clear import cache (useful for testing or memory management)
         */
        void clearImportCache();
    };
}