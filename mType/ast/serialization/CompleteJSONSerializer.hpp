#pragma once
#include "../ASTNode.hpp"
#include "SerializationFormat.hpp"
#include <string>
#include <memory>
#include <unordered_set>
#include <filesystem>
#include <json.hpp>

namespace ast::serialization
{
    /**
     * @brief Complete JSON-based AST serializer for all node types
     * Provides human-readable AST serialization for debugging and transparent caching
     */
    class CompleteJSONSerializer
    {
    private:
        // JSON library using nlohmann/json
        using json = nlohmann::json;

        // Import resolution tracking
        std::unordered_set<std::string> processedImports;
        std::string baseDirectory;
        std::vector<std::unique_ptr<ASTNode>> loadedImports; // Keep imported ASTs alive

        // Core serialization
        json serializeNode(const std::shared_ptr<ASTNode>& node);
        json serializeNode(ASTNode* node);
        NodeType getNodeType(const ASTNode* node);
        std::string getNodeTypeName(NodeType type);

        // Expression node serialization
        json serializeIntegerNode(const ASTNode* node);
        json serializeFloatNode(const ASTNode* node);
        json serializeStringNode(const ASTNode* node);
        json serializeBoolNode(const ASTNode* node);
        json serializeNullNode(const ASTNode* node);
        json serializeVariableNode(const ASTNode* node);
        json serializeBinaryExpNode(const ASTNode* node);
        json serializeUnaryExpNode(const ASTNode* node);
        json serializeTernaryExpNode(const ASTNode* node);
        json serializeIndexAccessNode(const ASTNode* node);
        json serializeArrayCreationNode(const ASTNode* node);

        // Statement node serialization
        json serializeProgramNode(const ASTNode* node);
        json serializeBlockNode(const ASTNode* node);
        json serializeDeclarationNode(const ASTNode* node);
        json serializeAssignmentNode(const ASTNode* node);
        json serializeMemberAssignmentNode(const ASTNode* node);
        json serializeIndexAssignmentNode(const ASTNode* node);
        json serializeReturnNode(const ASTNode* node);
        json serializeImportNode(const ASTNode* node);
        json serializeIfNode(const ASTNode* node);
        json serializeWhileNode(const ASTNode* node);
        json serializeDoWhileNode(const ASTNode* node);
        json serializeForNode(const ASTNode* node);
        json serializeForEachNode(const ASTNode* node);
        json serializeBreakNode(const ASTNode* node);
        json serializeContinueNode(const ASTNode* node);
        json serializeSwitchNode(const ASTNode* node);
        json serializeCaseNode(const ASTNode* node);
        json serializeDefaultCaseNode(const ASTNode* node);

        // Function node serialization
        json serializeFunctionNode(const ASTNode* node);
        json serializeFunctionCallNode(const ASTNode* node);

        // Class node serialization
        json serializeClassNode(const ASTNode* node);
        json serializeNewNode(const ASTNode* node);
        json serializeMemberAccessNode(const ASTNode* node);
        json serializeMethodCallNode(const ASTNode* node);
        json serializeFieldNode(const ASTNode* node);
        json serializeMethodNode(const ASTNode* node);
        json serializeConstructorNode(const ASTNode* node);

        // Helper serialization methods
        json serializeValueType(ValueType type);
        json serializeBinaryOperator(uint8_t op);
        json serializeUnaryOperator(uint8_t op);
        json serializeGenericTypeParameters(const std::vector<std::string>& params);
        json serializeParameters(const std::vector<std::pair<std::string, ValueType>>& params);
        json serializeSourceLocation(const ASTNode* node);

        // Import resolution helpers
        void resolveImportsRecursively(const ASTNode* root);
        std::unique_ptr<ASTNode> loadImportedAST(const std::string& importPath);
        std::string resolveImportPath(const std::string& importPath);

    public:
        CompleteJSONSerializer();
        ~CompleteJSONSerializer() = default;

        /**
         * @brief Serialize AST to JSON string
         */
        std::string serialize(const std::shared_ptr<ASTNode>& root);

        /**
         * @brief Serialize AST and save to file
         */
        bool serializeToFile(const std::shared_ptr<ASTNode>& root, const std::string& filename);

        /**
         * @brief Serialize AST with import resolution - processes all imported files recursively
         */
        bool serializeToFileWithImports(const std::shared_ptr<ASTNode>& root, const std::string& filename, const std::string& baseDirectory);
    };
}