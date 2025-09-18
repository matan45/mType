#include "ASTSerializer.hpp"
#include "../nodes/expressions/IntegerNode.hpp"
#include "../nodes/expressions/FloatNode.hpp"
#include "../nodes/expressions/StringNode.hpp"
#include "../nodes/expressions/BoolNode.hpp"
#include "../nodes/expressions/NullNode.hpp"
#include "../nodes/expressions/VariableNode.hpp"
#include "../nodes/expressions/BinaryExpNode.hpp"
#include "../nodes/expressions/UnaryExpNode.hpp"
#include "../nodes/expressions/TernaryExpNode.hpp"
#include "../nodes/expressions/ArrayLiteralNode.hpp"
#include "../nodes/expressions/MapLiteralNode.hpp"
#include "../nodes/expressions/IndexAccessNode.hpp"

#include "../nodes/statements/ProgramNode.hpp"
#include "../nodes/statements/BlockNode.hpp"
#include "../nodes/statements/DeclarationNode.hpp"
#include "../nodes/statements/AssignmentNode.hpp"
#include "../nodes/statements/MemberAssignmentNode.hpp"
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
#include "../nodes/statements/ImportNode.hpp"
#include "../nodes/statements/NativeFunctionNode.hpp"

#include "../nodes/functions/FunctionNode.hpp"
#include "../nodes/functions/FunctionCallNode.hpp"
#include "../nodes/functions/ReturnNode.hpp"

#include "../nodes/classes/ClassNode.hpp"
#include "../nodes/classes/MethodNode.hpp"
#include "../nodes/classes/FieldNode.hpp"
#include "../nodes/classes/ConstructorNode.hpp"
#include "../nodes/classes/NewNode.hpp"
#include "../nodes/classes/MemberAccessNode.hpp"
#include "../nodes/classes/MethodCallNode.hpp"

#include <iostream>
#include <vector>
#include <typeinfo>
#include "../../token/TokenType.hpp"

namespace ast::serialization
{
    ASTSerializer::ASTSerializer() : contentSize(0)
    {
    }

    ASTSerializer::~ASTSerializer()
    {
        if (stream.is_open())
        {
            stream.close();
        }
    }

    bool ASTSerializer::serialize(const ASTNode* root, const std::string& filePath)
    {
        stream.open(filePath, std::ios::binary | std::ios::out);
        if (!stream.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }

        try
        {
            // Reserve space for header
            FileHeader header = {};
            stream.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

            // Serialize the AST
            size_t contentStart = stream.tellp();
            serializeNode(root);
            size_t contentEnd = stream.tellp();

            contentSize = static_cast<uint32_t>(contentEnd - contentStart);

            // Go back and write the actual header
            stream.seekp(0);
            header.magic = MAGIC_NUMBER;
            header.version = CURRENT_VERSION;
            header.size = contentSize;
            header.checksum = 0; // For now, we'll implement checksum later
            stream.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

            stream.close();
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during serialization: " << e.what() << std::endl;
            stream.close();
            return false;
        }
    }

    void ASTSerializer::serializeNode(const ASTNode* node)
    {
        if (!node)
        {
            return;
        }

        NodeType type = getNodeType(node);
        NodeHeader header;
        header.type = type;
        header.line = node->getLocation().getLine();
        header.column = node->getLocation().getColumn();
        header.dataSize = 0; // Will be updated after writing node data

        size_t headerPos = stream.tellp();
        stream.write(reinterpret_cast<const char*>(&header), sizeof(NodeHeader));
        size_t dataStart = stream.tellp();

        // Serialize node-specific data based on type
        switch (type)
        {
            case NodeType::INTEGER_NODE:
                serializeIntegerNode(dynamic_cast<const nodes::expressions::IntegerNode*>(node));
                break;
            case NodeType::FLOAT_NODE:
                serializeFloatNode(dynamic_cast<const nodes::expressions::FloatNode*>(node));
                break;
            case NodeType::STRING_NODE:
                serializeStringNode(dynamic_cast<const nodes::expressions::StringNode*>(node));
                break;
            case NodeType::BOOL_NODE:
                serializeBoolNode(dynamic_cast<const nodes::expressions::BoolNode*>(node));
                break;
            case NodeType::NULL_NODE:
                serializeNullNode(dynamic_cast<const nodes::expressions::NullNode*>(node));
                break;
            case NodeType::VARIABLE_NODE:
                serializeVariableNode(dynamic_cast<const nodes::expressions::VariableNode*>(node));
                break;
            case NodeType::PROGRAM_NODE:
                serializeProgramNode(dynamic_cast<const nodes::statements::ProgramNode*>(node));
                break;
            case NodeType::BLOCK_NODE:
                serializeBlockNode(dynamic_cast<const nodes::statements::BlockNode*>(node));
                break;
            case NodeType::CLASS_NODE:
                serializeClassNode(dynamic_cast<const nodes::classes::ClassNode*>(node));
                break;
            case NodeType::DECLARATION_NODE:
                serializeDeclarationNode(dynamic_cast<const nodes::statements::DeclarationNode*>(node));
                break;
            case NodeType::ASSIGNMENT_NODE:
                serializeAssignmentNode(dynamic_cast<const nodes::statements::AssignmentNode*>(node));
                break;
            case NodeType::FUNCTION_NODE:
                serializeFunctionNode(dynamic_cast<const nodes::functions::FunctionNode*>(node));
                break;
            case NodeType::FUNCTION_CALL_NODE:
                serializeFunctionCallNode(dynamic_cast<const nodes::functions::FunctionCallNode*>(node));
                break;
            case NodeType::BINARY_EXP_NODE:
                serializeBinaryExpNode(dynamic_cast<const nodes::expressions::BinaryExpNode*>(node));
                break;
            case NodeType::UNARY_EXP_NODE:
                serializeUnaryExpNode(dynamic_cast<const nodes::expressions::UnaryExpNode*>(node));
                break;
            case NodeType::RETURN_NODE:
                serializeReturnNode(dynamic_cast<const nodes::functions::ReturnNode*>(node));
                break;
            case NodeType::IMPORT_NODE:
                serializeImportNode(dynamic_cast<const nodes::statements::ImportNode*>(node));
                break;
            // Add cases for other node types as we implement them
            default:
                std::cerr << "Warning: Unsupported node type for serialization: " << static_cast<int>(type) << std::endl;
                break;
        }

        size_t dataEnd = stream.tellp();
        header.dataSize = static_cast<uint32_t>(dataEnd - dataStart);

        // Update header with actual data size
        stream.seekp(headerPos);
        stream.write(reinterpret_cast<const char*>(&header), sizeof(NodeHeader));
        stream.seekp(dataEnd);
    }

    // Helper methods for writing primitive types
    void ASTSerializer::writeUInt8(uint8_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
    }

    void ASTSerializer::writeUInt16(uint16_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(uint16_t));
    }

    void ASTSerializer::writeUInt32(uint32_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
    }

    void ASTSerializer::writeInt32(int32_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(int32_t));
    }

    void ASTSerializer::writeDouble(double value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(double));
    }

    void ASTSerializer::writeString(const std::string& value)
    {
        uint32_t length = static_cast<uint32_t>(value.length());
        writeUInt32(length);
        stream.write(value.c_str(), length);
    }

    void ASTSerializer::writeBool(bool value)
    {
        uint8_t boolValue = value ? 1 : 0;
        writeUInt8(boolValue);
    }

    void ASTSerializer::writeSourceLocation(const errors::SourceLocation& location)
    {
        writeUInt32(location.getLine());
        writeUInt32(location.getColumn());
    }

    // Node-specific serialization implementations
    void ASTSerializer::serializeIntegerNode(const nodes::expressions::IntegerNode* node)
    {
        if (node)
        {
            writeInt32(node->getValue());
        }
    }

    void ASTSerializer::serializeFloatNode(const nodes::expressions::FloatNode* node)
    {
        if (node)
        {
            writeDouble(node->getValue());
        }
    }

    void ASTSerializer::serializeStringNode(const nodes::expressions::StringNode* node)
    {
        if (node)
        {
            writeString(node->getValue());
        }
    }

    void ASTSerializer::serializeBoolNode(const nodes::expressions::BoolNode* node)
    {
        if (node)
        {
            writeBool(node->getValue());
        }
    }

    void ASTSerializer::serializeNullNode(const nodes::expressions::NullNode* node)
    {
        // Null node has no additional data
    }

    void ASTSerializer::serializeVariableNode(const nodes::expressions::VariableNode* node)
    {
        if (node)
        {
            writeString(node->getName());
        }
    }

    void ASTSerializer::serializeProgramNode(const nodes::statements::ProgramNode* node)
    {
        if (node)
        {
            serializeChildNodes(node->getStatements());
        }
    }

    void ASTSerializer::serializeBlockNode(const nodes::statements::BlockNode* node)
    {
        if (node)
        {
            serializeChildNodes(node->getStatements());
        }
    }

    void ASTSerializer::serializeClassNode(const nodes::classes::ClassNode* node)
    {
        if (node)
        {
            writeString(node->getClassName());
            serializeChildNodes(node->getFields());
            serializeChildNodes(node->getConstructors());
            serializeChildNodes(node->getMethods());
        }
    }

    void ASTSerializer::serializeDeclarationNode(const nodes::statements::DeclarationNode* node)
    {
        if (node)
        {
            writeString(node->getVariableName());
            writeUInt8(static_cast<uint8_t>(node->getType())); // Serialize ValueType as uint8_t
            writeBool(node->isFinal());
            writeBool(node->isStatic());

            // Serialize initializer (optional)
            if (node->getInitializer())
            {
                writeBool(true); // Has initializer
                serializeNode(node->getInitializer());
            }
            else
            {
                writeBool(false); // No initializer
            }
        }
    }

    void ASTSerializer::serializeAssignmentNode(const nodes::statements::AssignmentNode* node)
    {
        if (node)
        {
            writeString(node->getVariableName());
            writeUInt8(static_cast<uint8_t>(node->getVariableType())); // Serialize ValueType as uint8_t
            writeString(node->getClassName());
            writeBool(node->getIsFinal());
            writeBool(node->getIsStatic());

            // Serialize value
            if (node->getValue())
            {
                serializeNode(node->getValue());
            }
        }
    }

    void ASTSerializer::serializeFunctionNode(const nodes::functions::FunctionNode* node)
    {
        if (node)
        {
            writeString(node->getName());
            writeUInt8(static_cast<uint8_t>(node->getReturnType()));

            // Serialize parameters
            const auto& parameters = node->getParameters();
            writeUInt32(static_cast<uint32_t>(parameters.size()));
            for (const auto& param : parameters)
            {
                writeString(param.first);  // parameter name
                writeUInt8(static_cast<uint8_t>(param.second)); // parameter type
            }

            // Serialize function body
            if (node->getBodyPtr())
            {
                serializeNode(node->getBodyPtr());
            }
        }
    }

    void ASTSerializer::serializeFunctionCallNode(const nodes::functions::FunctionCallNode* node)
    {
        if (node)
        {
            writeString(node->getFunctionName());
            serializeChildNodes(node->getArguments());
        }
    }

    void ASTSerializer::serializeBinaryExpNode(const nodes::expressions::BinaryExpNode* node)
    {
        if (node)
        {
            writeUInt8(tokenTypeToBinaryOperator(node->getOperator()));
            serializeNode(node->getLeft());
            serializeNode(node->getRight());
        }
    }

    void ASTSerializer::serializeUnaryExpNode(const nodes::expressions::UnaryExpNode* node)
    {
        if (node)
        {
            writeUInt8(tokenTypeToUnaryOperator(node->getOperator()));
            writeUInt8(unaryPositionToByte(node));
            serializeNode(node->getOperand());
        }
    }

    void ASTSerializer::serializeReturnNode(const nodes::functions::ReturnNode* node)
    {
        if (node)
        {
            writeBool(node->hasReturnValue());
            if (node->hasReturnValue())
            {
                serializeNode(node->getReturnValue());
            }
        }
    }

    void ASTSerializer::serializeChildNodes(const std::vector<std::unique_ptr<ASTNode>>& children)
    {
        uint32_t count = static_cast<uint32_t>(children.size());
        writeUInt32(count);

        for (const auto& child : children)
        {
            serializeNode(child.get());
        }
    }

    void ASTSerializer::serializeChildNode(const std::unique_ptr<ASTNode>& child)
    {
        serializeNode(child.get());
    }

    NodeType ASTSerializer::getNodeType(const ASTNode* node)
    {
        // Use dynamic_cast to determine the actual type
        if (dynamic_cast<const nodes::expressions::IntegerNode*>(node))
            return NodeType::INTEGER_NODE;
        if (dynamic_cast<const nodes::expressions::FloatNode*>(node))
            return NodeType::FLOAT_NODE;
        if (dynamic_cast<const nodes::expressions::StringNode*>(node))
            return NodeType::STRING_NODE;
        if (dynamic_cast<const nodes::expressions::BoolNode*>(node))
            return NodeType::BOOL_NODE;
        if (dynamic_cast<const nodes::expressions::NullNode*>(node))
            return NodeType::NULL_NODE;
        if (dynamic_cast<const nodes::expressions::VariableNode*>(node))
            return NodeType::VARIABLE_NODE;
        if (dynamic_cast<const nodes::statements::ProgramNode*>(node))
            return NodeType::PROGRAM_NODE;
        if (dynamic_cast<const nodes::statements::BlockNode*>(node))
            return NodeType::BLOCK_NODE;
        if (dynamic_cast<const nodes::classes::ClassNode*>(node))
            return NodeType::CLASS_NODE;
        if (dynamic_cast<const nodes::statements::DeclarationNode*>(node))
            return NodeType::DECLARATION_NODE;
        if (dynamic_cast<const nodes::statements::AssignmentNode*>(node))
            return NodeType::ASSIGNMENT_NODE;
        if (dynamic_cast<const nodes::functions::FunctionNode*>(node))
            return NodeType::FUNCTION_NODE;
        if (dynamic_cast<const nodes::functions::FunctionCallNode*>(node))
            return NodeType::FUNCTION_CALL_NODE;
        if (dynamic_cast<const nodes::expressions::BinaryExpNode*>(node))
            return NodeType::BINARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::UnaryExpNode*>(node))
            return NodeType::UNARY_EXP_NODE;
        if (dynamic_cast<const nodes::functions::ReturnNode*>(node))
            return NodeType::RETURN_NODE;
        if (dynamic_cast<const nodes::statements::ImportNode*>(node))
            return NodeType::IMPORT_NODE;

        // Add more type checks as we implement them
        throw std::runtime_error("Unknown node type for serialization");
    }

    // Helper function implementations
    uint8_t ASTSerializer::tokenTypeToBinaryOperator(token::TokenType tokenType)
    {
        switch (tokenType)
        {
            case token::TokenType::PLUS: return static_cast<uint8_t>(BinaryOperator::ADD);
            case token::TokenType::MINUS: return static_cast<uint8_t>(BinaryOperator::SUBTRACT);
            case token::TokenType::MULTIPLY: return static_cast<uint8_t>(BinaryOperator::MULTIPLY);
            case token::TokenType::DIVIDE: return static_cast<uint8_t>(BinaryOperator::DIVIDE);
            case token::TokenType::MODULO: return static_cast<uint8_t>(BinaryOperator::MODULO);
            case token::TokenType::EQUALS: return static_cast<uint8_t>(BinaryOperator::EQUAL);
            case token::TokenType::NOT_EQUALS: return static_cast<uint8_t>(BinaryOperator::NOT_EQUAL);
            case token::TokenType::LESS: return static_cast<uint8_t>(BinaryOperator::LESS_THAN);
            case token::TokenType::LESS_EQUALS: return static_cast<uint8_t>(BinaryOperator::LESS_EQUAL);
            case token::TokenType::GREATER: return static_cast<uint8_t>(BinaryOperator::GREATER_THAN);
            case token::TokenType::GREATER_EQUALS: return static_cast<uint8_t>(BinaryOperator::GREATER_EQUAL);
            case token::TokenType::AND: return static_cast<uint8_t>(BinaryOperator::LOGICAL_AND);
            case token::TokenType::OR: return static_cast<uint8_t>(BinaryOperator::LOGICAL_OR);
            case token::TokenType::ASSIGN: return static_cast<uint8_t>(BinaryOperator::ASSIGN);
            case token::TokenType::PLUS_ASSIGN: return static_cast<uint8_t>(BinaryOperator::PLUS_ASSIGN);
            case token::TokenType::MINUS_ASSIGN: return static_cast<uint8_t>(BinaryOperator::MINUS_ASSIGN);
            case token::TokenType::MULTIPLY_ASSIGN: return static_cast<uint8_t>(BinaryOperator::MULTIPLY_ASSIGN);
            case token::TokenType::DIVIDE_ASSIGN: return static_cast<uint8_t>(BinaryOperator::DIVIDE_ASSIGN);
            case token::TokenType::MODULO_ASSIGN: return static_cast<uint8_t>(BinaryOperator::MODULO_ASSIGN);
            default:
                throw std::runtime_error("Unknown binary operator token type");
        }
    }

    uint8_t ASTSerializer::tokenTypeToUnaryOperator(token::TokenType tokenType)
    {
        switch (tokenType)
        {
            case token::TokenType::MINUS: return static_cast<uint8_t>(UnaryOperator::MINUS);
            case token::TokenType::PLUS: return static_cast<uint8_t>(UnaryOperator::PLUS);
            case token::TokenType::NOT: return static_cast<uint8_t>(UnaryOperator::NOT);
            case token::TokenType::INCREMENT: return static_cast<uint8_t>(UnaryOperator::PRE_INCREMENT);
            case token::TokenType::DECREMENT: return static_cast<uint8_t>(UnaryOperator::PRE_DECREMENT);
            default:
                throw std::runtime_error("Unknown unary operator token type");
        }
    }

    uint8_t ASTSerializer::unaryPositionToByte(const nodes::expressions::UnaryExpNode* node)
    {
        return node->isPrefix() ? static_cast<uint8_t>(UnaryPosition::PREFIX) : static_cast<uint8_t>(UnaryPosition::POSTFIX);
    }

    // Placeholder implementations for other node types
    void ASTSerializer::serializeTernaryExpNode(const nodes::expressions::TernaryExpNode* node) {}
    void ASTSerializer::serializeArrayLiteralNode(const nodes::expressions::ArrayLiteralNode* node) {}
    void ASTSerializer::serializeMapLiteralNode(const nodes::expressions::MapLiteralNode* node) {}
    void ASTSerializer::serializeIndexAccessNode(const nodes::expressions::IndexAccessNode* node) {}
    void ASTSerializer::serializeMemberAssignmentNode(const nodes::statements::MemberAssignmentNode* node) {}
    void ASTSerializer::serializeIfNode(const nodes::statements::IfNode* node) {}
    void ASTSerializer::serializeWhileNode(const nodes::statements::WhileNode* node) {}
    void ASTSerializer::serializeDoWhileNode(const nodes::statements::DoWhileNode* node) {}
    void ASTSerializer::serializeForNode(const nodes::statements::ForNode* node) {}
    void ASTSerializer::serializeForEachNode(const nodes::statements::ForEachNode* node) {}
    void ASTSerializer::serializeBreakNode(const nodes::statements::BreakNode* node) {}
    void ASTSerializer::serializeContinueNode(const nodes::statements::ContinueNode* node) {}
    void ASTSerializer::serializeSwitchNode(const nodes::statements::SwitchNode* node) {}
    void ASTSerializer::serializeCaseNode(const nodes::statements::CaseNode* node) {}
    void ASTSerializer::serializeDefaultCaseNode(const nodes::statements::DefaultCaseNode* node) {}
    void ASTSerializer::serializeImportNode(const nodes::statements::ImportNode* node)
    {
        // Serialize the file path
        writeString(node->getFilePath());

        // Serialize the number of imported declarations
        const auto& declarations = node->getImportedDeclarations();
        writeUInt32(static_cast<uint32_t>(declarations.size()));

        // Serialize each imported declaration
        for (const auto& declaration : declarations)
        {
            serializeNode(declaration.get());
        }
    }
    void ASTSerializer::serializeNativeFunctionNode(const nodes::statements::NativeFunctionNode* node) {}
    void ASTSerializer::serializeMethodNode(const nodes::classes::MethodNode* node) {}
    void ASTSerializer::serializeFieldNode(const nodes::classes::FieldNode* node) {}
    void ASTSerializer::serializeConstructorNode(const nodes::classes::ConstructorNode* node) {}
    void ASTSerializer::serializeNewNode(const nodes::classes::NewNode* node) {}
    void ASTSerializer::serializeMemberAccessNode(const nodes::classes::MemberAccessNode* node) {}
    void ASTSerializer::serializeMethodCallNode(const nodes::classes::MethodCallNode* node) {}

    uint32_t ASTSerializer::calculateCRC32(const std::vector<uint8_t>& data)
    {
        // Simple CRC32 implementation - for production use a proper CRC32 library
        return 0;
    }
}