#pragma once
#include "SerializationFormat.hpp"
#include "../ASTNode.hpp"
#include "../GenericTypeParameter.hpp"
#include "../../parser/TypeParser.hpp"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace token { enum class TokenType; }
namespace ast::nodes::expressions { class UnaryExpNode; }

namespace ast::serialization
{
    class ASTSerializer
    {
    private:
        std::ofstream stream;
        uint32_t contentSize;

        // Helper methods for writing primitive types
        void writeUInt8(uint8_t value);
        void writeUInt16(uint16_t value);
        void writeUInt32(uint32_t value);
        void writeInt32(int32_t value);
        void writeDouble(double value);
        void writeString(const std::string& value);
        void writeBool(bool value);

        // Helper method for writing source location
        void writeSourceLocation(const errors::SourceLocation& location);

        // CRC32 calculation for checksum
        uint32_t calculateCRC32(const std::vector<uint8_t>& data);

    public:
        ASTSerializer();
        ~ASTSerializer();

        // Main serialization methods
        bool serialize(const ASTNode* root, const std::string& filePath);


        // Node-specific serialization methods
        void serializeNode(const ASTNode* node);

        // Expression node serialization
        void serializeIntegerNode(const nodes::expressions::IntegerNode* node);
        void serializeFloatNode(const nodes::expressions::FloatNode* node);
        void serializeStringNode(const nodes::expressions::StringNode* node);
        void serializeBoolNode(const nodes::expressions::BoolNode* node);
        void serializeNullNode(const nodes::expressions::NullNode* node);
        void serializeVariableNode(const nodes::expressions::VariableNode* node);
        void serializeBinaryExpNode(const nodes::expressions::BinaryExpNode* node);
        void serializeUnaryExpNode(const nodes::expressions::UnaryExpNode* node);
        void serializeTernaryExpNode(const nodes::expressions::TernaryExpNode* node);
        void serializeArrayLiteralNode(const nodes::expressions::ArrayLiteralNode* node);
        void serializeMapLiteralNode(const nodes::expressions::MapLiteralNode* node);
        void serializeIndexAccessNode(const nodes::expressions::IndexAccessNode* node);

        // Statement node serialization
        void serializeProgramNode(const nodes::statements::ProgramNode* node);
        void serializeBlockNode(const nodes::statements::BlockNode* node);
        void serializeDeclarationNode(const nodes::statements::DeclarationNode* node);
        void serializeAssignmentNode(const nodes::statements::AssignmentNode* node);
        void serializeMemberAssignmentNode(const nodes::statements::MemberAssignmentNode* node);
        void serializeIfNode(const nodes::statements::IfNode* node);
        void serializeWhileNode(const nodes::statements::WhileNode* node);
        void serializeDoWhileNode(const nodes::statements::DoWhileNode* node);
        void serializeForNode(const nodes::statements::ForNode* node);
        void serializeForEachNode(const nodes::statements::ForEachNode* node);
        void serializeBreakNode(const nodes::statements::BreakNode* node);
        void serializeContinueNode(const nodes::statements::ContinueNode* node);
        void serializeSwitchNode(const nodes::statements::SwitchNode* node);
        void serializeCaseNode(const nodes::statements::CaseNode* node);
        void serializeDefaultCaseNode(const nodes::statements::DefaultCaseNode* node);
        void serializeImportNode(const nodes::statements::ImportNode* node);
        void serializeNativeFunctionNode(const nodes::statements::NativeFunctionNode* node);

        // Function node serialization
        void serializeFunctionNode(const nodes::functions::FunctionNode* node);
        void serializeFunctionCallNode(const nodes::functions::FunctionCallNode* node);
        void serializeReturnNode(const nodes::functions::ReturnNode* node);

        // Class node serialization
        void serializeClassNode(const nodes::classes::ClassNode* node);
        void serializeMethodNode(const nodes::classes::MethodNode* node);
        void serializeFieldNode(const nodes::classes::FieldNode* node);
        void serializeConstructorNode(const nodes::classes::ConstructorNode* node);
        void serializeNewNode(const nodes::classes::NewNode* node);
        void serializeMemberAccessNode(const nodes::classes::MemberAccessNode* node);
        void serializeMethodCallNode(const nodes::classes::MethodCallNode* node);

        // Utility methods
        void serializeChildNodes(const std::vector<std::unique_ptr<ASTNode>>& children);
        void serializeChildNode(const std::unique_ptr<ASTNode>& child);

    private:
        // Internal helper to get node type from ASTNode
        NodeType getNodeType(const ASTNode* node);

        // Helper functions to convert TokenType to serialization enums
        uint8_t tokenTypeToBinaryOperator(token::TokenType tokenType);
        uint8_t tokenTypeToUnaryOperator(token::TokenType tokenType);
        uint8_t unaryPositionToByte(const nodes::expressions::UnaryExpNode* node);

        // Helper function to convert value::ValueType to serialization ValueType
        uint8_t valueTypeToSerializationType(value::ValueType valueType);

        // TypeInfo serialization methods
        void writeTypeInfo(const parser::TypeInfo& typeInfo);

        // Generic type parameter serialization methods
        void writeGenericTypeParameter(const ast::GenericTypeParameter& param);
        void writeGenericTypeParameters(const std::vector<ast::GenericTypeParameter>& params);
    };
}