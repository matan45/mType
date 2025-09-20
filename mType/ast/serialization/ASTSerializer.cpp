#include "ASTSerializer.hpp"
#include <fstream>
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
            // Reserve space for header (write placeholder)
            FileHeader header = {};
            stream.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

            // Serialize the AST content
            size_t contentStart = stream.tellp();
            serializeNode(root);
            size_t contentEnd = stream.tellp();
            contentSize = static_cast<uint32_t>(contentEnd - contentStart);

            // Close the stream to ensure data is written
            stream.close();

            // Now read the content back to calculate checksum
            std::ifstream readStream(filePath, std::ios::binary);
            if (!readStream.is_open())
            {
                std::cerr << "Failed to read file for checksum calculation" << std::endl;
                return false;
            }

            // Skip header and read content
            readStream.seekg(sizeof(FileHeader));
            std::vector<uint8_t> contentData(contentSize);
            readStream.read(reinterpret_cast<char*>(contentData.data()), contentSize);
            readStream.close();

            // Calculate checksum
            uint32_t checksum = calculateCRC32(contentData);

            // Reopen file and update header with checksum
            std::fstream updateStream(filePath, std::ios::binary | std::ios::in | std::ios::out);
            if (!updateStream.is_open())
            {
                std::cerr << "Failed to open file for header update" << std::endl;
                return false;
            }

            // Write the complete header
            header.magic = MAGIC_NUMBER;
            header.version = CURRENT_VERSION;
            header.size = contentSize;
            header.checksum = checksum;
            updateStream.seekp(0);
            updateStream.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
            updateStream.close();

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
        case NodeType::MEMBER_ASSIGNMENT_NODE:
            serializeMemberAssignmentNode(dynamic_cast<const nodes::statements::MemberAssignmentNode*>(node));
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
        case NodeType::TERNARY_EXP_NODE:
            serializeTernaryExpNode(dynamic_cast<const nodes::expressions::TernaryExpNode*>(node));
            break;
        case NodeType::ARRAY_LITERAL_NODE:
            serializeArrayLiteralNode(dynamic_cast<const nodes::expressions::ArrayLiteralNode*>(node));
            break;
        case NodeType::MAP_LITERAL_NODE:
            serializeMapLiteralNode(dynamic_cast<const nodes::expressions::MapLiteralNode*>(node));
            break;
        case NodeType::INDEX_ACCESS_NODE:
            serializeIndexAccessNode(dynamic_cast<const nodes::expressions::IndexAccessNode*>(node));
            break;
        case NodeType::RETURN_NODE:
            serializeReturnNode(dynamic_cast<const nodes::functions::ReturnNode*>(node));
            break;
        case NodeType::IMPORT_NODE:
            serializeImportNode(dynamic_cast<const nodes::statements::ImportNode*>(node));
            break;
        case NodeType::IF_NODE:
            serializeIfNode(dynamic_cast<const nodes::statements::IfNode*>(node));
            break;
        case NodeType::WHILE_NODE:
            serializeWhileNode(dynamic_cast<const nodes::statements::WhileNode*>(node));
            break;
        case NodeType::DO_WHILE_NODE:
            serializeDoWhileNode(dynamic_cast<const nodes::statements::DoWhileNode*>(node));
            break;
        case NodeType::FOR_NODE:
            serializeForNode(dynamic_cast<const nodes::statements::ForNode*>(node));
            break;
        case NodeType::FOR_EACH_NODE:
            serializeForEachNode(dynamic_cast<const nodes::statements::ForEachNode*>(node));
            break;
        case NodeType::BREAK_NODE:
            serializeBreakNode(dynamic_cast<const nodes::statements::BreakNode*>(node));
            break;
        case NodeType::CONTINUE_NODE:
            serializeContinueNode(dynamic_cast<const nodes::statements::ContinueNode*>(node));
            break;
        case NodeType::SWITCH_NODE:
            serializeSwitchNode(dynamic_cast<const nodes::statements::SwitchNode*>(node));
            break;
        case NodeType::CASE_NODE:
            serializeCaseNode(dynamic_cast<const nodes::statements::CaseNode*>(node));
            break;
        case NodeType::DEFAULT_CASE_NODE:
            serializeDefaultCaseNode(dynamic_cast<const nodes::statements::DefaultCaseNode*>(node));
            break;
        case NodeType::NEW_NODE:
            serializeNewNode(dynamic_cast<const nodes::classes::NewNode*>(node));
            break;
        case NodeType::MEMBER_ACCESS_NODE:
            serializeMemberAccessNode(dynamic_cast<const nodes::classes::MemberAccessNode*>(node));
            break;
        case NodeType::METHOD_CALL_NODE:
            serializeMethodCallNode(dynamic_cast<const nodes::classes::MethodCallNode*>(node));
            break;
        case NodeType::FIELD_NODE:
            serializeFieldNode(dynamic_cast<const nodes::classes::FieldNode*>(node));
            break;
        case NodeType::METHOD_NODE:
            serializeMethodNode(dynamic_cast<const nodes::classes::MethodNode*>(node));
            break;
        case NodeType::CONSTRUCTOR_NODE:
            serializeConstructorNode(dynamic_cast<const nodes::classes::ConstructorNode*>(node));
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
        writeString(location.getFilename());
        writeUInt32(location.getLine());
        writeUInt32(location.getColumn());
    }

    uint32_t ASTSerializer::calculateCRC32(const std::vector<uint8_t>& data)
    {
        // Simple checksum calculation for now
        // TODO: Implement proper CRC32 algorithm if needed
        uint32_t checksum = 0;
        for (uint8_t byte : data)
        {
            checksum ^= byte;
            checksum = (checksum << 1) | (checksum >> 31);
        }
        return checksum;
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
                writeString(param.first); // parameter name
                writeUInt8(static_cast<uint8_t>(param.second)); // parameter type
            }

            // Serialize function body
            bool hasBody = (node->getBodyPtr() != nullptr);
            writeBool(hasBody);
            if (hasBody)
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
        if (dynamic_cast<const nodes::statements::MemberAssignmentNode*>(node))
            return NodeType::MEMBER_ASSIGNMENT_NODE;
        if (dynamic_cast<const nodes::functions::FunctionNode*>(node))
            return NodeType::FUNCTION_NODE;
        if (dynamic_cast<const nodes::functions::FunctionCallNode*>(node))
            return NodeType::FUNCTION_CALL_NODE;
        if (dynamic_cast<const nodes::expressions::BinaryExpNode*>(node))
            return NodeType::BINARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::UnaryExpNode*>(node))
            return NodeType::UNARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::TernaryExpNode*>(node))
            return NodeType::TERNARY_EXP_NODE;
        if (dynamic_cast<const nodes::expressions::ArrayLiteralNode*>(node))
            return NodeType::ARRAY_LITERAL_NODE;
        if (dynamic_cast<const nodes::expressions::MapLiteralNode*>(node))
            return NodeType::MAP_LITERAL_NODE;
        if (dynamic_cast<const nodes::expressions::IndexAccessNode*>(node))
            return NodeType::INDEX_ACCESS_NODE;
        if (dynamic_cast<const nodes::functions::ReturnNode*>(node))
            return NodeType::RETURN_NODE;
        if (dynamic_cast<const nodes::statements::ImportNode*>(node))
            return NodeType::IMPORT_NODE;
        if (dynamic_cast<const nodes::statements::IfNode*>(node))
            return NodeType::IF_NODE;
        if (dynamic_cast<const nodes::statements::WhileNode*>(node))
            return NodeType::WHILE_NODE;
        if (dynamic_cast<const nodes::statements::DoWhileNode*>(node))
            return NodeType::DO_WHILE_NODE;
        if (dynamic_cast<const nodes::statements::ForNode*>(node))
            return NodeType::FOR_NODE;
        if (dynamic_cast<const nodes::statements::ForEachNode*>(node))
            return NodeType::FOR_EACH_NODE;
        if (dynamic_cast<const nodes::statements::BreakNode*>(node))
            return NodeType::BREAK_NODE;
        if (dynamic_cast<const nodes::statements::ContinueNode*>(node))
            return NodeType::CONTINUE_NODE;
        if (dynamic_cast<const nodes::statements::SwitchNode*>(node))
            return NodeType::SWITCH_NODE;
        if (dynamic_cast<const nodes::statements::CaseNode*>(node))
            return NodeType::CASE_NODE;
        if (dynamic_cast<const nodes::statements::DefaultCaseNode*>(node))
            return NodeType::DEFAULT_CASE_NODE;
        if (dynamic_cast<const nodes::classes::NewNode*>(node))
            return NodeType::NEW_NODE;
        if (dynamic_cast<const nodes::classes::MemberAccessNode*>(node))
            return NodeType::MEMBER_ACCESS_NODE;
        if (dynamic_cast<const nodes::classes::MethodCallNode*>(node))
            return NodeType::METHOD_CALL_NODE;
        if (dynamic_cast<const nodes::classes::FieldNode*>(node))
            return NodeType::FIELD_NODE;
        if (dynamic_cast<const nodes::classes::MethodNode*>(node))
            return NodeType::METHOD_NODE;
        if (dynamic_cast<const nodes::classes::ConstructorNode*>(node))
            return NodeType::CONSTRUCTOR_NODE;

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
        return node->isPrefix()
                   ? static_cast<uint8_t>(UnaryPosition::PREFIX)
                   : static_cast<uint8_t>(UnaryPosition::POSTFIX);
    }

    // Placeholder implementations for other node types
    void ASTSerializer::serializeTernaryExpNode(const nodes::expressions::TernaryExpNode* node)
    {
        // Serialize the condition expression
        serializeNode(node->getCondition());

        // Serialize the true expression
        serializeNode(node->getTrueExpression());

        // Serialize the false expression
        serializeNode(node->getFalseExpression());
    }

    void ASTSerializer::serializeArrayLiteralNode(const nodes::expressions::ArrayLiteralNode* node)
    {
        // Serialize the element type
        writeUInt8(static_cast<uint8_t>(node->getElementType()));

        // Serialize the number of elements
        const auto& elements = node->getElements();
        writeUInt32(static_cast<uint32_t>(elements.size()));

        // Serialize each element
        for (const auto& element : elements)
        {
            serializeNode(element.get());
        }
    }

    void ASTSerializer::serializeMapLiteralNode(const nodes::expressions::MapLiteralNode* node)
    {
        // Serialize the key and value types
        writeUInt8(static_cast<uint8_t>(node->getKeyType()));
        writeUInt8(static_cast<uint8_t>(node->getValueType()));

        // Serialize the number of key-value pairs
        const auto& pairs = node->getKeyValuePairs();
        writeUInt32(static_cast<uint32_t>(pairs.size()));

        // Serialize each key-value pair
        for (const auto& pair : pairs)
        {
            serializeNode(pair.first.get()); // key
            serializeNode(pair.second.get()); // value
        }
    }

    void ASTSerializer::serializeIndexAccessNode(const nodes::expressions::IndexAccessNode* node)
    {
        // Serialize the collection expression
        serializeNode(node->getCollection());

        // Serialize the index expression
        serializeNode(node->getIndex());
    }

    void ASTSerializer::serializeMemberAssignmentNode(const nodes::statements::MemberAssignmentNode* node)
    {
        if (node)
        {
            // Serialize the object
            serializeNode(node->getObject());

            // Serialize the member name
            writeString(node->getMemberName());

            // Serialize the value
            serializeNode(node->getValue());
        }
    }

    void ASTSerializer::serializeIfNode(const nodes::statements::IfNode* node)
    {
        // Serialize the condition expression
        serializeNode(node->getCondition());

        // Serialize the then statement
        serializeNode(node->getThenStatement());

        // Serialize whether there's an else statement
        writeBool(node->hasElseStatement());

        // Serialize the else statement if it exists
        if (node->hasElseStatement())
        {
            serializeNode(node->getElseStatement());
        }
    }

    void ASTSerializer::serializeWhileNode(const nodes::statements::WhileNode* node)
    {
        // Serialize the condition expression
        serializeNode(node->getCondition());

        // Serialize the body statement
        serializeNode(node->getBody());
    }

    void ASTSerializer::serializeDoWhileNode(const nodes::statements::DoWhileNode* node)
    {
        if (node)
        {
            // Serialize the body statement first (executed before condition check)
            serializeNode(node->getBody());

            // Serialize the condition expression
            serializeNode(node->getCondition());
        }
    }

    void ASTSerializer::serializeForNode(const nodes::statements::ForNode* node)
    {
        // Serialize the initialization statement
        serializeNode(node->getInitialization());

        // Serialize the condition expression
        serializeNode(node->getCondition());

        // Serialize the update statement
        serializeNode(node->getUpdate());

        // Serialize the body statement
        serializeNode(node->getBody());
    }

    void ASTSerializer::serializeForEachNode(const nodes::statements::ForEachNode* node)
    {
        if (node)
        {
            // Serialize the variable name
            writeString(node->getVariableName());

            // Serialize the complete variable type information
            writeTypeInfo(node->getVariableTypeInfo());

            // Serialize the collection
            serializeNode(node->getCollection());

            // Serialize the body
            serializeNode(node->getBody());
        }
    }

    void ASTSerializer::serializeBreakNode(const nodes::statements::BreakNode* node)
    {
        // BreakNode has no additional data beyond location (which is handled by NodeHeader)
    }

    void ASTSerializer::serializeContinueNode(const nodes::statements::ContinueNode* node)
    {
        // ContinueNode has no additional data beyond location (which is handled by NodeHeader)
    }

    void ASTSerializer::serializeSwitchNode(const nodes::statements::SwitchNode* node)
    {
        if (node)
        {
            // Serialize the switch expression
            serializeNode(node->getExpression());

            // Serialize the cases
            serializeChildNodes(node->getCases());
        }
    }

    void ASTSerializer::serializeCaseNode(const nodes::statements::CaseNode* node)
    {
        if (node)
        {
            // Serialize the case value
            serializeNode(node->getValue());

            // Serialize the statements
            serializeChildNodes(node->getStatements());
        }
    }

    void ASTSerializer::serializeDefaultCaseNode(const nodes::statements::DefaultCaseNode* node)
    {
        if (node)
        {
            // Serialize the statements
            serializeChildNodes(node->getStatements());
        }
    }

    void ASTSerializer::serializeImportNode(const nodes::statements::ImportNode* node)
    {
        // Convert the file path to .mtc format for serialized imports
        std::string originalPath = node->getFilePath();
        std::string serializedPath = originalPath;

        // Convert .mt to .mtc
        if (originalPath.ends_with(".mt")) {
            serializedPath = originalPath + "c"; // .mt -> .mtc
        }

        // Serialize the converted file path
        writeString(serializedPath);

        // Serialize the number of imported declarations
        const auto& declarations = node->getImportedDeclarations();
        writeUInt32(static_cast<uint32_t>(declarations.size()));

        // Serialize each imported declaration
        for (const auto& declaration : declarations)
        {
            serializeNode(declaration.get());
        }
    }

    void ASTSerializer::serializeNativeFunctionNode(const nodes::statements::NativeFunctionNode* node)
    {
    }

    void ASTSerializer::serializeMethodNode(const nodes::classes::MethodNode* node)
    {
        if (node)
        {
            // Serialize the method name
            writeString(node->getName());

            // Serialize the return type
            writeUInt8(valueTypeToSerializationType(node->getReturnType()));

            // Serialize the parameters
            const auto& parameters = node->getParameters();
            writeUInt32(static_cast<uint32_t>(parameters.size()));

            for (const auto& param : parameters)
            {
                writeString(param.first); // parameter name
                writeUInt8(valueTypeToSerializationType(param.second)); // parameter type
            }

            // Serialize the static flag
            writeBool(node->getIsStatic());

            // Serialize whether there's a body
            bool hasBody = (node->getBodyPtr() != nullptr);
            writeBool(hasBody);

            // Serialize the body if it exists
            if (hasBody)
            {
                serializeNode(node->getBodyPtr());
            }
        }
    }

    void ASTSerializer::serializeFieldNode(const nodes::classes::FieldNode* node)
    {
        // Serialize the field name
        writeString(node->getName());

        // Serialize the field type
        writeUInt8(static_cast<uint8_t>(node->getType()));

        // Serialize flags
        writeBool(node->getIsStatic());
        writeBool(node->getIsFinal());

        // Serialize whether there's an initial value
        bool hasInitialValue = node->hasInitialValue();
        writeBool(hasInitialValue);

        // Serialize the initial value if it exists
        if (hasInitialValue)
        {
            serializeNode(node->getInitialValue());
        }
    }

    void ASTSerializer::serializeConstructorNode(const nodes::classes::ConstructorNode* node)
    {
        if (node)
        {
            // Serialize the parameters
            const auto& parameters = node->getParameters();
            writeUInt32(static_cast<uint32_t>(parameters.size()));

            for (const auto& param : parameters)
            {
                writeString(param.first); // parameter name
                writeUInt8(valueTypeToSerializationType(param.second)); // parameter type
            }

            // Serialize whether there's a body
            bool hasBody = (node->getBodyPtr() != nullptr);
            writeBool(hasBody);

            // Serialize the body if it exists
            if (hasBody)
            {
                serializeNode(node->getBodyPtr());
            }
        }
    }

    void ASTSerializer::serializeNewNode(const nodes::classes::NewNode* node)
    {
        // Serialize the class name
        writeString(node->getClassName());

        // Serialize the number of arguments
        const auto& arguments = node->getArguments();
        writeUInt32(static_cast<uint32_t>(arguments.size()));

        // Serialize each argument
        for (const auto& argument : arguments)
        {
            serializeNode(argument.get());
        }
    }

    void ASTSerializer::serializeMemberAccessNode(const nodes::classes::MemberAccessNode* node)
    {
        // Write whether there's an object
        bool hasObject = (node->getObject() != nullptr);
        writeBool(hasObject);

        // Serialize the object if it exists
        if (hasObject)
        {
            serializeNode(node->getObject());
        }

        // Serialize the member name
        writeString(node->getMemberName());

        // Serialize the static access flag
        writeBool(node->getIsStaticAccess());
    }

    void ASTSerializer::serializeMethodCallNode(const nodes::classes::MethodCallNode* node)
    {
        // Write whether there's an object
        bool hasObject = (node->getObject() != nullptr);
        writeBool(hasObject);

        // Serialize the object if it exists
        if (hasObject)
        {
            serializeNode(node->getObject());
        }

        // Serialize the method name
        writeString(node->getMethodName());

        // Serialize the static call flag
        writeBool(node->getIsStaticCall());

        // Serialize the number of arguments
        const auto& arguments = node->getArguments();
        writeUInt32(static_cast<uint32_t>(arguments.size()));

        // Serialize each argument
        for (const auto& argument : arguments)
        {
            serializeNode(argument.get());
        }
    }

    uint8_t ASTSerializer::valueTypeToSerializationType(value::ValueType valueType)
    {
        switch (valueType)
        {
        case value::ValueType::INT: return static_cast<uint8_t>(ast::serialization::ValueType::INT);
        case value::ValueType::FLOAT: return static_cast<uint8_t>(ast::serialization::ValueType::FLOAT);
        case value::ValueType::STRING: return static_cast<uint8_t>(ast::serialization::ValueType::STRING);
        case value::ValueType::BOOL: return static_cast<uint8_t>(ast::serialization::ValueType::BOOL);
        case value::ValueType::OBJECT: return static_cast<uint8_t>(ast::serialization::ValueType::OBJECT);
        case value::ValueType::ARRAY: return static_cast<uint8_t>(ast::serialization::ValueType::ARRAY);
        case value::ValueType::MAP: return static_cast<uint8_t>(ast::serialization::ValueType::MAP);
        case value::ValueType::SET: return static_cast<uint8_t>(ast::serialization::ValueType::SET);
        case value::ValueType::STACK: return static_cast<uint8_t>(ast::serialization::ValueType::STACK);
        case value::ValueType::QUEUE: return static_cast<uint8_t>(ast::serialization::ValueType::QUEUE);
        case value::ValueType::NULL_TYPE: return static_cast<uint8_t>(ast::serialization::ValueType::NULL_VALUE);
        case value::ValueType::VOID: return static_cast<uint8_t>(ast::serialization::ValueType::VOID);
        default:
            throw std::runtime_error(
                "Unknown value::ValueType: " + std::to_string(static_cast<int>(valueType)));
        }
    }

    void ASTSerializer::writeTypeInfo(const parser::TypeInfo& typeInfo)
    {
        // Serialize base type
        writeUInt8(valueTypeToSerializationType(typeInfo.baseType));

        // Serialize class name
        writeString(typeInfo.className);

        // Serialize element type info for collections
        if (typeInfo.elementTypeInfo.has_value()) {
            writeUInt8(1); // Has element type
            writeTypeInfo(*typeInfo.elementTypeInfo.value());
        } else {
            writeUInt8(0); // No element type
        }

        // Serialize key type info for maps
        if (typeInfo.keyTypeInfo.has_value()) {
            writeUInt8(1); // Has key type
            writeTypeInfo(*typeInfo.keyTypeInfo.value());
        } else {
            writeUInt8(0); // No key type
        }

        // Serialize value type info for maps
        if (typeInfo.valueTypeInfo.has_value()) {
            writeUInt8(1); // Has value type
            writeTypeInfo(*typeInfo.valueTypeInfo.value());
        } else {
            writeUInt8(0); // No value type
        }

        // Serialize legacy fields for backward compatibility
        if (typeInfo.elementType.has_value()) {
            writeUInt8(1); // Has legacy element type
            writeUInt8(valueTypeToSerializationType(typeInfo.elementType.value()));
        } else {
            writeUInt8(0); // No legacy element type
        }

        if (typeInfo.keyType.has_value()) {
            writeUInt8(1); // Has legacy key type
            writeUInt8(valueTypeToSerializationType(typeInfo.keyType.value()));
        } else {
            writeUInt8(0); // No legacy key type
        }

        if (typeInfo.valueType.has_value()) {
            writeUInt8(1); // Has legacy value type
            writeUInt8(valueTypeToSerializationType(typeInfo.valueType.value()));
        } else {
            writeUInt8(0); // No legacy value type
        }

        // Serialize class names
        writeString(typeInfo.elementClassName.value_or(""));
        writeString(typeInfo.keyClassName.value_or(""));
        writeString(typeInfo.valueClassName.value_or(""));
    }
}
