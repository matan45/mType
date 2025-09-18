#pragma once
#include "SerializationFormat.hpp"
#include "../ASTNode.hpp"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace token { enum class TokenType; }
namespace ast::nodes::expressions { enum class UnaryPosition; }

namespace ast::serialization
{
    class ASTDeserializer
    {
    private:
        std::ifstream stream;

        // Helper methods for reading primitive types
        uint8_t readUInt8();
        uint16_t readUInt16();
        uint32_t readUInt32();
        int32_t readInt32();
        double readDouble();
        std::string readString();
        bool readBool();

        // Helper method for reading source location
        errors::SourceLocation readSourceLocation();

        // CRC32 validation
        bool validateChecksum(uint32_t expectedChecksum, const std::vector<uint8_t>& data);

    public:
        ASTDeserializer();
        ~ASTDeserializer();

        // Main deserialization method
        std::unique_ptr<ASTNode> deserialize(const std::string& filePath);

        // Node-specific deserialization methods
        std::unique_ptr<ASTNode> deserializeNode();

        // Expression node deserialization
        std::unique_ptr<ASTNode> deserializeIntegerNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeFloatNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeStringNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeBoolNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeNullNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeVariableNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeBinaryExpNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeUnaryExpNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeTernaryExpNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeArrayLiteralNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeMapLiteralNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeIndexAccessNode(const NodeHeader& header);

        // Statement node deserialization
        std::unique_ptr<ASTNode> deserializeProgramNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeBlockNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeDeclarationNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeAssignmentNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeMemberAssignmentNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeIfNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeWhileNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeDoWhileNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeForNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeForEachNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeBreakNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeContinueNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeSwitchNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeCaseNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeDefaultCaseNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeImportNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeNativeFunctionNode(const NodeHeader& header);

        // Function node deserialization
        std::unique_ptr<ASTNode> deserializeFunctionNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeFunctionCallNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeReturnNode(const NodeHeader& header);

        // Class node deserialization
        std::unique_ptr<ASTNode> deserializeClassNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeMethodNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeFieldNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeConstructorNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeNewNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeMemberAccessNode(const NodeHeader& header);
        std::unique_ptr<ASTNode> deserializeMethodCallNode(const NodeHeader& header);

        // Utility methods
        std::vector<std::unique_ptr<ASTNode>> deserializeChildNodes();
        std::unique_ptr<ASTNode> deserializeChildNode();

    private:
        // Internal helper to create source location from line/column
        errors::SourceLocation createSourceLocation(uint32_t line, uint32_t column);

        // Helper functions to convert serialization enums to TokenType
        token::TokenType binaryOperatorToTokenType(uint8_t binaryOp);
        token::TokenType unaryOperatorToTokenType(uint8_t unaryOp);
        nodes::expressions::UnaryPosition byteToUnaryPosition(uint8_t position);
    };
}