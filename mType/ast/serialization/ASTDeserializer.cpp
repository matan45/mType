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
#include "../nodes/expressions/ArrayCreationNode.hpp"
#include "../nodes/expressions/IndexAccessNode.hpp"

#include "../nodes/statements/ProgramNode.hpp"
#include "../nodes/statements/BlockNode.hpp"
#include "../nodes/statements/DeclarationNode.hpp"
#include "../nodes/statements/AssignmentNode.hpp"
#include "../nodes/statements/MemberAssignmentNode.hpp"
#include "../nodes/statements/IndexAssignmentNode.hpp"
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
    using namespace value;

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
        std::cout << "[LOG] ASTDeserializer::deserialize - Starting deserialization of: " << filePath << std::endl;

        stream.open(filePath, std::ios::binary | std::ios::in);
        if (!stream.is_open())
        {
            std::cerr << "[LOG] ASTDeserializer::deserialize - Failed to open file for reading: " << filePath << std::endl;
            return nullptr;
        }

        try
        {
            std::cout << "[LOG] ASTDeserializer::deserialize - Reading file header..." << std::endl;
            // Read and validate header
            FileHeader header;
            stream.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

            std::cout << "[LOG] ASTDeserializer::deserialize - Validating magic number..." << std::endl;
            if (header.magic != MAGIC_NUMBER)
            {
                std::cerr << "[LOG] ASTDeserializer::deserialize - Invalid file format: wrong magic number" << std::endl;
                return nullptr;
            }

            std::cout << "[LOG] ASTDeserializer::deserialize - Checking version..." << std::endl;
            if (header.version != CURRENT_VERSION)
            {
                std::cerr << "Unsupported format version: " << header.version << std::endl;
                return nullptr;
            }

            std::cout << "[LOG] ASTDeserializer::deserialize - Reading content data (size: " << header.size << ")..." << std::endl;
            // Read content data for checksum validation
            std::vector<uint8_t> contentData(header.size);
            stream.read(reinterpret_cast<char*>(contentData.data()), header.size);

            std::cout << "[LOG] ASTDeserializer::deserialize - Validating checksum..." << std::endl;
            // Validate checksum
            if (!validateChecksum(header.checksum, contentData))
            {
                std::cerr << "Checksum validation failed: file may be corrupted" << std::endl;
                return nullptr;
            }

            std::cout << "[LOG] ASTDeserializer::deserialize - Resetting stream position..." << std::endl;
            // Reset stream position to start of content for deserialization
            stream.seekg(sizeof(FileHeader));

            std::cout << "[LOG] ASTDeserializer::deserialize - Starting root node deserialization..." << std::endl;
            // Deserialize the root node
            auto rootNode = deserializeNode();
            std::cout << "[LOG] ASTDeserializer::deserialize - Root node deserialization completed" << std::endl;
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
        std::cout << "[LOG] ASTDeserializer::deserializeNode - Reading node header..." << std::endl;
        // Read node header
        NodeHeader header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(NodeHeader));

        if (stream.eof() || stream.fail())
        {
            std::cout << "[LOG] ASTDeserializer::deserializeNode - End of file or read failure reached" << std::endl;
            return nullptr;
        }

        std::cout << "[LOG] ASTDeserializer::deserializeNode - Processing node type: " << static_cast<int>(header.type) << std::endl;

        // Deserialize node based on type
        switch (header.type)
        {
        case NodeType::INTEGER_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing INTEGER_NODE" << std::endl;
            return deserializeIntegerNode(header);
        case NodeType::FLOAT_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing FLOAT_NODE" << std::endl;
            return deserializeFloatNode(header);
        case NodeType::STRING_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing STRING_NODE" << std::endl;
            return deserializeStringNode(header);
        case NodeType::BOOL_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing BOOL_NODE" << std::endl;
            return deserializeBoolNode(header);
        case NodeType::NULL_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing NULL_NODE" << std::endl;
            return deserializeNullNode(header);
        case NodeType::VARIABLE_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing VARIABLE_NODE" << std::endl;
            return deserializeVariableNode(header);
        case NodeType::PROGRAM_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing PROGRAM_NODE" << std::endl;
            return deserializeProgramNode(header);
        case NodeType::BLOCK_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing BLOCK_NODE" << std::endl;
            return deserializeBlockNode(header);
        case NodeType::CLASS_NODE:
            return deserializeClassNode(header);
        case NodeType::DECLARATION_NODE:
            return deserializeDeclarationNode(header);
        case NodeType::ASSIGNMENT_NODE:
            return deserializeAssignmentNode(header);
        case NodeType::MEMBER_ASSIGNMENT_NODE:
            return deserializeMemberAssignmentNode(header);
        case NodeType::INDEX_ASSIGNMENT_NODE:
            return deserializeIndexAssignmentNode(header);
        case NodeType::FUNCTION_NODE:
            return deserializeFunctionNode(header);
        case NodeType::FUNCTION_CALL_NODE:
            return deserializeFunctionCallNode(header);
        case NodeType::BINARY_EXP_NODE:
            return deserializeBinaryExpNode(header);
        case NodeType::UNARY_EXP_NODE:
            return deserializeUnaryExpNode(header);
        case NodeType::TERNARY_EXP_NODE:
            return deserializeTernaryExpNode(header);
        case NodeType::ARRAY_CREATION_NODE:
            return deserializeArrayCreationNode(header);
        case NodeType::INDEX_ACCESS_NODE:
            return deserializeIndexAccessNode(header);
        case NodeType::RETURN_NODE:
            return deserializeReturnNode(header);
        case NodeType::IMPORT_NODE:
            std::cout << "[LOG] ASTDeserializer::deserializeNode - Deserializing IMPORT_NODE" << std::endl;
            return deserializeImportNode(header);
        case NodeType::IF_NODE:
            return deserializeIfNode(header);
        case NodeType::WHILE_NODE:
            return deserializeWhileNode(header);
        case NodeType::DO_WHILE_NODE:
            return deserializeDoWhileNode(header);
        case NodeType::FOR_NODE:
            return deserializeForNode(header);
        case NodeType::FOR_EACH_NODE:
            return deserializeForEachNode(header);
        case NodeType::BREAK_NODE:
            return deserializeBreakNode(header);
        case NodeType::CONTINUE_NODE:
            return deserializeContinueNode(header);
        case NodeType::SWITCH_NODE:
            return deserializeSwitchNode(header);
        case NodeType::CASE_NODE:
            return deserializeCaseNode(header);
        case NodeType::DEFAULT_CASE_NODE:
            return deserializeDefaultCaseNode(header);
        case NodeType::NEW_NODE:
            return deserializeNewNode(header);
        case NodeType::MEMBER_ACCESS_NODE:
            return deserializeMemberAccessNode(header);
        case NodeType::METHOD_CALL_NODE:
            return deserializeMethodCallNode(header);
        case NodeType::FIELD_NODE:
            return deserializeFieldNode(header);
        case NodeType::METHOD_NODE:
            return deserializeMethodNode(header);
        case NodeType::CONSTRUCTOR_NODE:
            return deserializeConstructorNode(header);
        // Add more cases as we implement them
        default:
            throw std::runtime_error(
                "Unknown node type during deserialization: " + std::to_string(static_cast<int>(header.type)));
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
        std::string filename = readString();
        uint32_t line = readUInt32();
        uint32_t column = readUInt32();
        return createSourceLocation(filename, line, column);
    }

    errors::SourceLocation ASTDeserializer::createSourceLocation(uint32_t line, uint32_t column)
    {
        errors::SourceLocation location;
        location.setLine(line);
        location.setColumn(column);
        return location;
    }

    errors::SourceLocation ASTDeserializer::createSourceLocation(const std::string& filename, uint32_t line, uint32_t column)
    {
        errors::SourceLocation location;
        location.setFilename(filename);
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
        return std::make_unique<nodes::expressions::FloatNode>(static_cast<float>(value), location);
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
        std::cout << "[LOG] ASTDeserializer::deserializeProgramNode - Starting..." << std::endl;
        auto statements = deserializeChildNodes();
        std::cout << "[LOG] ASTDeserializer::deserializeProgramNode - Child nodes deserialized, creating program node..." << std::endl;
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto programNode = std::make_unique<nodes::statements::ProgramNode>(location);
        programNode->setStatements(std::move(statements));
        std::cout << "[LOG] ASTDeserializer::deserializeProgramNode - Completed" << std::endl;
        return std::move(programNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBlockNode(const NodeHeader& header)
    {
        std::cout << "[LOG] ASTDeserializer::deserializeBlockNode - Starting..." << std::endl;
        auto statements = deserializeChildNodes();
        std::cout << "[LOG] ASTDeserializer::deserializeBlockNode - Child nodes deserialized, creating block node..." << std::endl;
        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto blockNode = std::make_unique<nodes::statements::BlockNode>(location);
        blockNode->setStatements(std::move(statements));
        std::cout << "[LOG] ASTDeserializer::deserializeBlockNode - Completed" << std::endl;
        return std::move(blockNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeClassNode(const NodeHeader& header)
    {
        std::string className = readString();

        // Read generic type parameters
        auto genericParameters = readGenericTypeParameters();

        auto fields = deserializeChildNodes();
        auto constructors = deserializeChildNodes();
        auto methods = deserializeChildNodes();

        errors::SourceLocation location = createSourceLocation(header.line, header.column);
        auto classNode = std::make_unique<nodes::classes::ClassNode>(className, genericParameters, location);

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
        bool hasBody = readBool();
        std::unique_ptr<ASTNode> body = nullptr;
        if (hasBody)
        {
            body = deserializeNode();
        }

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
        std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Starting..." << std::endl;
        uint32_t count = readUInt32();
        std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Reading " << count << " child nodes" << std::endl;
        std::vector<std::unique_ptr<ASTNode>> children;
        children.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Deserializing child " << (i + 1) << "/" << count << std::endl;
            auto child = deserializeNode();
            if (child)
            {
                std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Child " << (i + 1) << " deserialized successfully" << std::endl;
                children.push_back(std::move(child));
            }
            else
            {
                std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Child " << (i + 1) << " returned null, breaking loop" << std::endl;
                break; // Stop processing when we can't read more nodes
            }
        }

        std::cout << "[LOG] ASTDeserializer::deserializeChildNodes - Completed with " << children.size() << " children" << std::endl;
        return children;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeChildNode()
    {
        return deserializeNode();
    }

    bool ASTDeserializer::validateChecksum(uint32_t expectedChecksum, const std::vector<uint8_t>& data)
    {
        // Calculate checksum of the data using the same algorithm as serializer
        uint32_t calculatedChecksum = 0;
        for (uint8_t byte : data)
        {
            calculatedChecksum ^= byte;
            calculatedChecksum = (calculatedChecksum << 1) | (calculatedChecksum >> 31);
        }

        return calculatedChecksum == expectedChecksum;
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
    std::unique_ptr<ASTNode> ASTDeserializer::deserializeTernaryExpNode(const NodeHeader& header)
    {
        // Read the condition expression
        auto condition = deserializeNode();

        // Read the true expression
        auto trueExpression = deserializeNode();

        // Read the false expression
        auto falseExpression = deserializeNode();

        // Create the ternary expression node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::expressions::TernaryExpNode>(
            std::move(condition),
            std::move(trueExpression),
            std::move(falseExpression),
            location);
    }


    std::unique_ptr<ASTNode> ASTDeserializer::deserializeArrayCreationNode(const NodeHeader& header)
    {
        // Read the element type information
        parser::TypeInfo elementTypeInfo = readTypeInfo();

        // Read whether size is dynamic
        bool isDynamicSize = readBool();

        // Read the size expression
        auto sizeExpression = deserializeNode();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the ArrayCreationNode
        return std::make_unique<nodes::expressions::ArrayCreationNode>(
            elementTypeInfo,
            std::move(sizeExpression),
            location
        );
    }


    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIndexAccessNode(const NodeHeader& header)
    {
        // Read the collection expression
        auto collection = deserializeNode();

        // Read the index expression
        auto index = deserializeNode();

        // Create the index access node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::expressions::IndexAccessNode>(
            std::move(collection),
            std::move(index),
            location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMemberAssignmentNode(const NodeHeader& header)
    {
        // Read the object
        auto object = deserializeNode();

        // Read the member name
        std::string memberName = readString();

        // Read the value
        auto value = deserializeNode();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the MemberAssignmentNode
        return std::make_unique<nodes::statements::MemberAssignmentNode>(
            std::move(object),
            memberName,
            std::move(value),
            location
        );
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIndexAssignmentNode(const NodeHeader& header)
    {
        // Read the object
        auto object = deserializeNode();

        // Read the index
        auto index = deserializeNode();

        // Read the value
        auto value = deserializeNode();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the IndexAssignmentNode
        return std::make_unique<nodes::statements::IndexAssignmentNode>(
            std::move(object),
            std::move(index),
            std::move(value),
            location
        );
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeIfNode(const NodeHeader& header)
    {
        // Read the condition expression
        auto condition = deserializeNode();

        // Read the then statement
        auto thenStatement = deserializeNode();

        // Read whether there's an else statement
        bool hasElse = readBool();

        // Read the else statement if it exists
        std::unique_ptr<ASTNode> elseStatement = nullptr;
        if (hasElse)
        {
            elseStatement = deserializeNode();
        }

        // Create the if node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::statements::IfNode>(
            std::move(condition),
            std::move(thenStatement),
            std::move(elseStatement),
            location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeWhileNode(const NodeHeader& header)
    {
        // Read the condition expression
        auto condition = deserializeNode();

        // Read the body statement
        auto body = deserializeNode();

        // Create the while node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::statements::WhileNode>(
            std::move(condition),
            std::move(body),
            location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeDoWhileNode(const NodeHeader& header)
    {
        // Read the body statement first (as it was serialized first)
        auto body = deserializeNode();

        // Read the condition expression
        auto condition = deserializeNode();

        // Create the do-while node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::statements::DoWhileNode>(
            std::move(body),
            std::move(condition),
            location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeForNode(const NodeHeader& header)
    {
        // Read the initialization statement
        auto initialization = deserializeNode();

        // Read the condition expression
        auto condition = deserializeNode();

        // Read the update statement
        auto update = deserializeNode();

        // Read the body statement
        auto body = deserializeNode();

        // Create the for node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::statements::ForNode>(
            std::move(initialization),
            std::move(condition),
            std::move(update),
            std::move(body),
            location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeForEachNode(const NodeHeader& header)
    {
        // Read the variable name
        std::string variableName = readString();

        // Read the complete variable type information
        parser::TypeInfo variableTypeInfo = readTypeInfo();

        // Read the collection
        auto collection = deserializeNode();

        // Read the body
        auto body = deserializeNode();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the ForEachNode using the new constructor
        return std::make_unique<nodes::statements::ForEachNode>(
            variableName,
            variableTypeInfo,
            std::move(collection),
            std::move(body),
            location
        );
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeBreakNode(const NodeHeader& header)
    {
        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // BreakNode has no additional data
        return std::make_unique<nodes::statements::BreakNode>(location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeContinueNode(const NodeHeader& header)
    {
        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // ContinueNode has no additional data
        return std::make_unique<nodes::statements::ContinueNode>(location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeSwitchNode(const NodeHeader& header)
    {
        // Read the switch expression
        auto expression = deserializeNode();

        // Read the cases
        auto cases = deserializeChildNodes();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the SwitchNode
        auto switchNode = std::make_unique<nodes::statements::SwitchNode>(
            std::move(expression),
            location
        );

        // Add all cases to the switch node
        for (auto& caseNode : cases)
        {
            switchNode->addCase(std::move(caseNode));
        }

        return switchNode;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeCaseNode(const NodeHeader& header)
    {
        // Read the case value
        auto value = deserializeNode();

        // Read the statements
        auto statements = deserializeChildNodes();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the CaseNode
        auto caseNode = std::make_unique<nodes::statements::CaseNode>(
            std::move(value),
            location
        );

        // Add all statements to the case node
        for (auto& statement : statements)
        {
            caseNode->addStatement(std::move(statement));
        }

        return caseNode;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeDefaultCaseNode(const NodeHeader& header)
    {
        // Read the statements
        auto statements = deserializeChildNodes();

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the DefaultCaseNode
        auto defaultCaseNode = std::make_unique<nodes::statements::DefaultCaseNode>(location);

        // Add all statements to the default case node
        for (auto& statement : statements)
        {
            defaultCaseNode->addStatement(std::move(statement));
        }

        return defaultCaseNode;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeImportNode(const NodeHeader& header)
    {
        std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Starting..." << std::endl;
        // Read the file path
        std::string filePath = readString();
        std::cout << "[LOG] ASTDeserializer::deserializeImportNode - File path: " << filePath << std::endl;

        // Create the import node with the file path
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);
        auto importNode = std::make_unique<nodes::statements::ImportNode>(filePath, location);

        // Read the number of imported declarations
        uint32_t declarationCount = readUInt32();
        std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Reading " << declarationCount << " imported declarations" << std::endl;

        // Read each imported declaration
        for (uint32_t i = 0; i < declarationCount; ++i)
        {
            std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Deserializing declaration " << (i + 1) << "/" << declarationCount << std::endl;
            auto declaration = deserializeNode();
            if (declaration)
            {
                std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Declaration " << (i + 1) << " deserialized successfully" << std::endl;
                importNode->addImportedDeclaration(std::move(declaration));
            }
            else
            {
                std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Declaration " << (i + 1) << " returned null, breaking loop" << std::endl;
                break; // Stop processing when we can't read more nodes
            }
        }

        std::cout << "[LOG] ASTDeserializer::deserializeImportNode - Completed" << std::endl;
        return importNode;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNativeFunctionNode(const NodeHeader& header)
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMethodNode(const NodeHeader& header)
    {
        // Read the method name
        std::string methodName = readString();

        // Read the return type
        uint8_t serializationType = readUInt8();
        value::ValueType returnType = serializationValueTypeToValueType(
            static_cast<ast::serialization::ValueType>(serializationType));

        // Read the parameters
        uint32_t parameterCount = readUInt32();
        std::vector<std::pair<std::string, value::ValueType>> parameters;
        parameters.reserve(parameterCount);

        for (uint32_t i = 0; i < parameterCount; ++i)
        {
            std::string paramName = readString();
            uint8_t paramSerializationType = readUInt8();
            value::ValueType paramType = serializationValueTypeToValueType(
                static_cast<ast::serialization::ValueType>(paramSerializationType));
            parameters.emplace_back(paramName, paramType);
        }

        // Read the static flag
        bool isStatic = readBool();

        // Read generic type parameters (only for non-static methods)
        std::vector<ast::GenericTypeParameter> genericParameters;
        if (!isStatic)
        {
            genericParameters = readGenericTypeParameters();
        }

        // Read whether there's a body
        bool hasBody = readBool();

        // Read the body if it exists
        std::shared_ptr<ASTNode> body = nullptr;
        if (hasBody)
        {
            auto bodyNode = deserializeNode();
            body = std::shared_ptr<ASTNode>(bodyNode.release());
        }

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create the MethodNode
        auto methodNode = std::make_unique<nodes::classes::MethodNode>(
            methodName,
            returnType,
            parameters,
            body,
            isStatic,
            location
        );

        // Set generic type parameters if not static
        if (!isStatic && !genericParameters.empty())
        {
            methodNode->setGenericTypeParameters(genericParameters);
        }

        return std::move(methodNode);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeFieldNode(const NodeHeader& header)
    {
        // Read the field name
        std::string fieldName = readString();

        // Read the field type
        uint8_t serializationType = readUInt8();
        value::ValueType fieldType = serializationValueTypeToValueType(
            static_cast<ast::serialization::ValueType>(serializationType));

        // Read flags
        bool isStatic = readBool();
        bool isFinal = readBool();

        // Read whether there's an initial value
        bool hasInitialValue = readBool();

        // Read the initial value if it exists
        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (hasInitialValue)
        {
            initialValue = deserializeNode();
        }

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the FieldNode
        return std::make_unique<nodes::classes::FieldNode>(
            fieldName,
            fieldType,
            std::move(initialValue),
            isStatic,
            isFinal,
            location
        );
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeConstructorNode(const NodeHeader& header)
    {
        // Read the parameters
        uint32_t parameterCount = readUInt32();
        std::vector<std::pair<std::string, value::ValueType>> parameters;
        parameters.reserve(parameterCount);

        for (uint32_t i = 0; i < parameterCount; ++i)
        {
            std::string paramName = readString();
            uint8_t paramSerializationType = readUInt8();
            value::ValueType paramType = serializationValueTypeToValueType(
                static_cast<ast::serialization::ValueType>(paramSerializationType));
            parameters.emplace_back(paramName, paramType);
        }

        // Read whether there's a body
        bool hasBody = readBool();

        // Read the body if it exists
        std::shared_ptr<ASTNode> body = nullptr;
        if (hasBody)
        {
            auto bodyNode = deserializeNode();
            body = std::shared_ptr<ASTNode>(bodyNode.release());
        }

        // Create source location
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        // Create and return the ConstructorNode
        return std::make_unique<nodes::classes::ConstructorNode>(
            parameters,
            body,
            location
        );
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeNewNode(const NodeHeader& header)
    {
        // Read the class name
        std::string className = readString();

        // Read the number of arguments
        uint32_t argumentCount = readUInt32();

        // Read each argument
        std::vector<std::unique_ptr<ASTNode>> arguments;
        arguments.reserve(argumentCount);

        for (uint32_t i = 0; i < argumentCount; ++i)
        {
            auto argument = deserializeNode();
            if (argument)
            {
                arguments.push_back(std::move(argument));
            }
        }

        // Create the new node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::classes::NewNode>(className, std::move(arguments), location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMemberAccessNode(const NodeHeader& header)
    {
        // Read whether there's an object
        bool hasObject = readBool();

        // Read the object if it exists
        std::unique_ptr<ASTNode> object = nullptr;
        if (hasObject)
        {
            object = deserializeNode();
        }

        // Read the member name
        std::string memberName = readString();

        // Read the static access flag
        bool isStaticAccess = readBool();

        // Read full source location information (filename, line, column)
        errors::SourceLocation location = readSourceLocation();

        return std::make_unique<nodes::classes::MemberAccessNode>(
            std::move(object), memberName, isStaticAccess, location);
    }

    std::unique_ptr<ASTNode> ASTDeserializer::deserializeMethodCallNode(const NodeHeader& header)
    {
        // Read whether there's an object
        bool hasObject = readBool();

        // Read the object if it exists
        std::unique_ptr<ASTNode> object = nullptr;
        if (hasObject)
        {
            object = deserializeNode();
        }

        // Read the method name
        std::string methodName = readString();

        // Read the static call flag
        bool isStaticCall = readBool();

        // Read the number of arguments
        uint32_t argumentCount = readUInt32();

        // Read each argument
        std::vector<std::unique_ptr<ASTNode>> arguments;
        arguments.reserve(argumentCount);

        for (uint32_t i = 0; i < argumentCount; ++i)
        {
            auto argument = deserializeNode();
            if (argument)
            {
                arguments.push_back(std::move(argument));
            }
        }

        // Create the method call node
        SourceLocation location;
        location.setLine(header.line);
        location.setColumn(header.column);

        return std::make_unique<nodes::classes::MethodCallNode>(
            std::move(object), methodName, std::move(arguments), isStaticCall, location);
    }

    // Helper function to convert serialization ValueType to value::ValueType
    value::ValueType ASTDeserializer::serializationValueTypeToValueType(ast::serialization::ValueType serializationType)
    {
        switch (serializationType)
        {
        case ast::serialization::ValueType::INT: return value::ValueType::INT;
        case ast::serialization::ValueType::FLOAT: return value::ValueType::FLOAT;
        case ast::serialization::ValueType::STRING: return value::ValueType::STRING;
        case ast::serialization::ValueType::BOOL: return value::ValueType::BOOL;
        case ast::serialization::ValueType::OBJECT: return value::ValueType::OBJECT;
        // Collection types removed - now implemented in mType
        case ast::serialization::ValueType::NULL_VALUE: return value::ValueType::NULL_TYPE;
        case ast::serialization::ValueType::VOID: return value::ValueType::VOID;
        default:
            throw std::runtime_error(
                "Unknown serialization ValueType: " + std::to_string(static_cast<int>(serializationType)));
        }
    }

    parser::TypeInfo ASTDeserializer::readTypeInfo()
    {
        // Read base type
        uint8_t baseTypeSerializedValue = readUInt8();
        value::ValueType baseType = serializationValueTypeToValueType(
            static_cast<ast::serialization::ValueType>(baseTypeSerializedValue));

        // Read class name
        std::string className = readString();

        // Create TypeInfo with base type and class name
        parser::TypeInfo typeInfo(baseType, className);

        // Read element type info
        uint8_t hasElementType = readUInt8();
        if (hasElementType) {
            auto elementTypeInfo = std::make_shared<parser::TypeInfo>(readTypeInfo());
            // Collection-specific TypeInfo fields no longer exist
        }

        // Read key type info
        uint8_t hasKeyType = readUInt8();
        if (hasKeyType) {
            auto keyTypeInfo = std::make_shared<parser::TypeInfo>(readTypeInfo());
            // Collection-specific TypeInfo fields no longer exist
        }

        // Read value type info
        uint8_t hasValueType = readUInt8();
        if (hasValueType) {
            auto valueTypeInfo = std::make_shared<parser::TypeInfo>(readTypeInfo());
            // Collection-specific TypeInfo fields no longer exist
        }

        // Read legacy fields for backward compatibility
        uint8_t hasLegacyElementType = readUInt8();
        if (hasLegacyElementType) {
            uint8_t legacyElementTypeValue = readUInt8();
            // Legacy collection field no longer exists
        }

        uint8_t hasLegacyKeyType = readUInt8();
        if (hasLegacyKeyType) {
            uint8_t legacyKeyTypeValue = readUInt8();
            // Legacy collection field no longer exists
        }

        uint8_t hasLegacyValueType = readUInt8();
        if (hasLegacyValueType) {
            uint8_t legacyValueTypeValue = readUInt8();
            // Legacy collection field no longer exists
        }

        // Read class names
        std::string elementClassName = readString();
        std::string keyClassName = readString();
        std::string valueClassName = readString();

        // Legacy collection class name fields no longer exist
        if (!elementClassName.empty()) {
            // elementClassName no longer used
        }
        if (!keyClassName.empty()) {
            // keyClassName no longer used
        }
        if (!valueClassName.empty()) {
            // valueClassName no longer used
        }

        return typeInfo;
    }

    ast::GenericTypeParameter ASTDeserializer::readGenericTypeParameter()
    {
        // Read the parameter name
        std::string name = readString();

        // Read the constraints count and constraints
        uint32_t constraintsCount = readUInt32();
        std::vector<std::string> constraints;
        constraints.reserve(constraintsCount);

        for (uint32_t i = 0; i < constraintsCount; ++i)
        {
            constraints.push_back(readString());
        }

        // Read the source location
        errors::SourceLocation location = readSourceLocation();

        return ast::GenericTypeParameter(name, constraints, location);
    }

    std::vector<ast::GenericTypeParameter> ASTDeserializer::readGenericTypeParameters()
    {
        // Read the count of generic type parameters
        uint32_t count = readUInt32();
        std::vector<ast::GenericTypeParameter> params;
        params.reserve(count);

        // Read each parameter
        for (uint32_t i = 0; i < count; ++i)
        {
            params.push_back(readGenericTypeParameter());
        }

        return params;
    }
}
