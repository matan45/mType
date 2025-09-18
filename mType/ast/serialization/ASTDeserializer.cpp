#include "ASTDeserializer.hpp"
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
#include <stdexcept>
#include "../../errors/SourceLocation.hpp"
#include "../../token/TokenType.hpp"

namespace ast::serialization
{
    ASTDeserializer::ASTDeserializer()
    {
    }

    ASTDeserializer::~ASTDeserializer()
    {
        if (stream.is_open())
        {
            stream.close();
        }
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserialize(const std::string& filePath)
    {
        stream.open(filePath, std::ios::binary | std::ios::in);
        if (!stream.is_open())
        {
            std::cerr << "Failed to open file for reading: " << filePath << std::endl;
            return nullptr;
        }

        try
        {
            // Read and validate header
            FileHeader header;
            stream.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

            if (header.magic != MAGIC_NUMBER)
            {
                std::cerr << "Invalid file format: wrong magic number" << std::endl;
                return nullptr;
            }

            if (header.version != CURRENT_VERSION)
            {
                std::cerr << "Unsupported format version: " << header.version << std::endl;
                return nullptr;
            }

            // Deserialize the root node
            auto rootNode = deserializeNode();
            stream.close();
            return rootNode;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during deserialization: " << e.what() << std::endl;
            stream.close();
            return nullptr;
        }
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNode()
    {
        // Read node header
        NodeHeader header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(NodeHeader));

        if (stream.eof())
        {
            return nullptr;
        }

        // Deserialize node based on type
        switch (header.type)
        {
            case NodeType::INTEGER_NODE:
                return deserializeIntegerNode(header);
            case NodeType::FLOAT_NODE:
                return deserializeFloatNode(header);
            case NodeType::STRING_NODE:
                return deserializeStringNode(header);
            case NodeType::BOOL_NODE:
                return deserializeBoolNode(header);
            case NodeType::NULL_NODE:
                return deserializeNullNode(header);
            case NodeType::VARIABLE_NODE:
                return deserializeVariableNode(header);
            case NodeType::PROGRAM_NODE:
                return deserializeProgramNode(header);
            case NodeType::BLOCK_NODE:
                return deserializeBlockNode(header);
            case NodeType::CLASS_NODE:
                return deserializeClassNode(header);
            case NodeType::DECLARATION_NODE:
                return deserializeDeclarationNode(header);
            case NodeType::ASSIGNMENT_NODE:
                return deserializeAssignmentNode(header);
            case NodeType::FUNCTION_NODE:
                return deserializeFunctionNode(header);
            case NodeType::FUNCTION_CALL_NODE:
                return deserializeFunctionCallNode(header);
            case NodeType::BINARY_EXP_NODE:
                return deserializeBinaryExpNode(header);
            case NodeType::UNARY_EXP_NODE:
                return deserializeUnaryExpNode(header);
            case NodeType::RETURN_NODE:
                return deserializeReturnNode(header);
            case NodeType::IMPORT_NODE:
                return deserializeImportNode(header);
            // Add more cases as we implement them
            default:
                throw std::runtime_error("Unknown node type during deserialization: " + std::to_string(static_cast<int>(header.type)));
        }
    }

    // Helper methods for reading primitive types
    uint8_t ASTDeserializer::readUInt8()
    {
        uint8_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(uint8_t));
        return value;
    }

    uint16_t ASTDeserializer::readUInt16()
    {
        uint16_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(uint16_t));
        return value;
    }

    uint32_t ASTDeserializer::readUInt32()
    {
        uint32_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
        return value;
    }

    int32_t ASTDeserializer::readInt32()
    {
        int32_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(int32_t));
        return value;
    }

    double ASTDeserializer::readDouble()
    {
        double value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(double));
        return value;
    }

    std::string ASTDeserializer::readString()
    {
        uint32_t length = readUInt32();
        std::string value(length, '\0');
        stream.read(&value[0], length);
        return value;
    }

    bool ASTDeserializer::readBool()
    {
        uint8_t value = readUInt8();
        return value != 0;
    }

    errors::SourceLocation ASTDeserializer::readSourceLocation()
    {
        uint32_t line = readUInt32();
        uint32_t column = readUInt32();
        return createSourceLocation(line, column);
    }

    errors::SourceLocation ASTDeserializer::createSourceLocation(uint32_t line, uint32_t column)
    {
        errors::SourceLocation location;
        location.setLine(line);
        location.setColumn(column);
        return location;
    }

    // Node-specific deserialization implementations
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIntegerNode(const NodeHeader& header)
    {
        int32_t value = readInt32();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::IntegerNode>(value, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeFloatNode(const NodeHeader& header)
    {
        double value = readDouble();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::FloatNode>(value, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeStringNode(const NodeHeader& header)
    {
        std::string value = readString();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::StringNode>(value, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBoolNode(const NodeHeader& header)
    {
        bool value = readBool();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::BoolNode>(value, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNullNode(const NodeHeader& header)
    {
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::NullNode>(location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeVariableNode(const NodeHeader& header)
    {
        std::string name = readString();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::VariableNode>(name, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeProgramNode(const NodeHeader& header)
    {
        auto statements = deserializeChildNodes();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto programNode = std::make_unique<nodes::statements::ProgramNode>(location);
        programNode->setStatements(std::move(statements));
        return std::move(programNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBlockNode(const NodeHeader& header)
    {
        auto statements = deserializeChildNodes();
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto blockNode = std::make_unique<nodes::statements::BlockNode>(location);
        blockNode->setStatements(std::move(statements));
        return std::move(blockNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeClassNode(const NodeHeader& header)
    {
        std::string className = readString();
        auto fields = deserializeChildNodes();
        auto constructors = deserializeChildNodes();
        auto methods = deserializeChildNodes();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto classNode = std::make_unique<nodes::classes::ClassNode>(className, location);

        for (auto& field : fields)
        {
            classNode->addField(std::move(field));
        }

        for (auto& constructor : constructors)
        {
            classNode->addConstructor(std::move(constructor));
        }

        for (auto& method : methods)
        {
            classNode->addMethod(std::move(method));
        }

        return std::move(classNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeDeclarationNode(const NodeHeader& header)
    {
        std::string variableName = readString();
        value::ValueType type = static_cast<value::ValueType>(readUInt8());
        bool isFinal = readBool();
        bool isStatic = readBool();

        // Check if there's an initializer
        bool hasInitializer = readBool();
        std::unique_ptr<ASTNode> initializer = nullptr;
        if (hasInitializer)
        {
            initializer = deserializeNode();
        }

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::statements::DeclarationNode>(
            variableName, type, std::move(initializer), isFinal, isStatic, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeAssignmentNode(const NodeHeader& header)
    {
        std::string variableName = readString();
        value::ValueType type = static_cast<value::ValueType>(readUInt8());
        std::string className = readString();
        bool isFinal = readBool();
        bool isStatic = readBool();

        // Read the assigned value
        std::unique_ptr<ASTNode> value = deserializeNode();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::statements::AssignmentNode>(
            variableName, std::move(value), type, className, isFinal, isStatic, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeFunctionNode(const NodeHeader& header)
    {
        std::string functionName = readString();
        value::ValueType returnType = static_cast<value::ValueType>(readUInt8());

        // Deserialize parameters
        uint32_t paramCount = readUInt32();
        std::vector<std::pair<std::string, value::ValueType>> parameters;
        parameters.reserve(paramCount);

        for (uint32_t i = 0; i < paramCount; ++i)
        {
            std::string paramName = readString();
            value::ValueType paramType = static_cast<value::ValueType>(readUInt8());
            parameters.emplace_back(paramName, paramType);
        }

        // Deserialize function body
        std::unique_ptr<ASTNode> body = deserializeNode();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::functions::FunctionNode>(
            functionName, returnType, std::move(parameters), std::move(body), location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeFunctionCallNode(const NodeHeader& header)
    {
        std::string functionName = readString();
        auto arguments = deserializeChildNodes();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::functions::FunctionCallNode>(
            functionName, std::move(arguments), location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBinaryExpNode(const NodeHeader& header)
    {
        uint8_t operatorByte = readUInt8();
        token::TokenType operator_ = binaryOperatorToTokenType(operatorByte);

        auto left = deserializeNode();
        auto right = deserializeNode();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::BinaryExpNode>(
            std::move(left), operator_, std::move(right), location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeUnaryExpNode(const NodeHeader& header)
    {
        uint8_t operatorByte = readUInt8();
        uint8_t positionByte = readUInt8();

        token::TokenType operator_ = unaryOperatorToTokenType(operatorByte);
        nodes::expressions::UnaryPosition position = byteToUnaryPosition(positionByte);

        auto operand = deserializeNode();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::expressions::UnaryExpNode>(
            operator_, std::move(operand), position, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeReturnNode(const NodeHeader& header)
    {
        bool hasReturnValue = readBool();
        std::unique_ptr<ASTNode> returnValue = nullptr;

        if (hasReturnValue)
        {
            returnValue = deserializeNode();
        }

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        return std::make_unique<nodes::functions::ReturnNode>(std::move(returnValue), location);
    }

    std::vector<std::unique_ptr<ASTNode>> ASTDeserializer::deserializeChildNodes()
    {
        uint32_t count = readUInt32();
        std::vector<std::unique_ptr<ASTNode>> children;
        children.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            auto child = deserializeNode();
            if (child)
            {
                children.push_back(std::move(child));
            }
        }

        return children;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeChildNode()
    {
        return deserializeNode();
    }

    bool ASTDeserializer::validateChecksum(uint32_t expectedChecksum, const std::vector<uint8_t>& data)
    {
        // For now, always return true - implement proper CRC32 validation later
        return true;
    }

    // Helper function implementations
    token::TokenType ASTDeserializer::binaryOperatorToTokenType(uint8_t binaryOp)
    {
        switch (static_cast<BinaryOperator>(binaryOp))
        {
            case BinaryOperator::ADD: return token::TokenType::PLUS;
            case BinaryOperator::SUBTRACT: return token::TokenType::MINUS;
            case BinaryOperator::MULTIPLY: return token::TokenType::MULTIPLY;
            case BinaryOperator::DIVIDE: return token::TokenType::DIVIDE;
            case BinaryOperator::MODULO: return token::TokenType::MODULO;
            case BinaryOperator::EQUAL: return token::TokenType::EQUALS;
            case BinaryOperator::NOT_EQUAL: return token::TokenType::NOT_EQUALS;
            case BinaryOperator::LESS_THAN: return token::TokenType::LESS;
            case BinaryOperator::LESS_EQUAL: return token::TokenType::LESS_EQUALS;
            case BinaryOperator::GREATER_THAN: return token::TokenType::GREATER;
            case BinaryOperator::GREATER_EQUAL: return token::TokenType::GREATER_EQUALS;
            case BinaryOperator::LOGICAL_AND: return token::TokenType::AND;
            case BinaryOperator::LOGICAL_OR: return token::TokenType::OR;
            case BinaryOperator::ASSIGN: return token::TokenType::ASSIGN;
            case BinaryOperator::PLUS_ASSIGN: return token::TokenType::PLUS_ASSIGN;
            case BinaryOperator::MINUS_ASSIGN: return token::TokenType::MINUS_ASSIGN;
            case BinaryOperator::MULTIPLY_ASSIGN: return token::TokenType::MULTIPLY_ASSIGN;
            case BinaryOperator::DIVIDE_ASSIGN: return token::TokenType::DIVIDE_ASSIGN;
            case BinaryOperator::MODULO_ASSIGN: return token::TokenType::MODULO_ASSIGN;
            default:
                throw std::runtime_error("Unknown binary operator during deserialization");
        }
    }

    token::TokenType ASTDeserializer::unaryOperatorToTokenType(uint8_t unaryOp)
    {
        switch (static_cast<UnaryOperator>(unaryOp))
        {
            case UnaryOperator::MINUS: return token::TokenType::MINUS;
            case UnaryOperator::PLUS: return token::TokenType::PLUS;
            case UnaryOperator::NOT: return token::TokenType::NOT;
            case UnaryOperator::PRE_INCREMENT: return token::TokenType::INCREMENT;
            case UnaryOperator::PRE_DECREMENT: return token::TokenType::DECREMENT;
            default:
                throw std::runtime_error("Unknown unary operator during deserialization");
        }
    }

    nodes::expressions::UnaryPosition ASTDeserializer::byteToUnaryPosition(uint8_t position)
    {
        switch (static_cast<UnaryPosition>(position))
        {
            case UnaryPosition::PREFIX: return nodes::expressions::UnaryPosition::PREFIX;
            case UnaryPosition::POSTFIX: return nodes::expressions::UnaryPosition::POSTFIX;
            default:
                throw std::runtime_error("Unknown unary position during deserialization");
        }
    }

    // Placeholder implementations for other node types
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeTernaryExpNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeArrayLiteralNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMapLiteralNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIndexAccessNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMemberAssignmentNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIfNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeWhileNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeDoWhileNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeForNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeForEachNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBreakNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeContinueNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeSwitchNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeCaseNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeDefaultCaseNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeImportNode(const NodeHeader& header)
    {
        // Read the file path
        std::string filePath = readString();

        // Create the import node with the file path
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);
        auto importNode = std::make_unique<nodes::statements::ImportNode>(filePath, location);

        // Read the number of imported declarations
        uint32_t declarationCount = readUInt32();

        // Read each imported declaration
        for (uint32_t i = 0; i < declarationCount; ++i)
        {
            auto declaration = deserializeNode();
            if (declaration)
            {
                importNode->addImportedDeclaration(std::move(declaration));
            }
        }

        return importNode;
    }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNativeFunctionNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMethodNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeFieldNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeConstructorNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNewNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMemberAccessNode(const NodeHeader& header) { return nullptr; }
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMethodCallNode(const NodeHeader& header) { return nullptr; }
}