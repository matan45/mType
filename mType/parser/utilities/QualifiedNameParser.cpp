#include "QualifiedNameParser.hpp"
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/ParseException.hpp"

namespace parser
{
    std::string QualifiedNameParser::buildQualifiedName(const std::vector<std::string>& parts) noexcept
    {
        if (parts.empty()) return {};
        if (parts.size() == 1) return parts[0];

        // Pre-calculate total size to avoid reallocations
        size_t totalSize = parts[0].size();
        for (size_t i = 1; i < parts.size(); ++i) {
            totalSize += 2 + parts[i].size(); // "::" + part
        }

        // Single allocation with exact size
        std::string result;
        result.reserve(totalSize);
        result = parts[0];

        // Efficient appending without temporary strings
        for (size_t i = 1; i < parts.size(); ++i) {
            result.append("::");
            result.append(parts[i]);
        }

        return result;
    }

    std::vector<std::string> QualifiedNameParser::parseQualifiedIdentifierChain(
        TokenStream& stream,
        std::string_view initialName)
    {
        using namespace token;
        using namespace errors;

        std::vector<std::string> parts = {std::string(initialName)};

        // We expect the stream to be positioned at the first identifier after the initial ::
        // Add that identifier to the parts
        parts.push_back(stream.current().stringValue.getString());
        stream.advance();

        // Continue parsing if there are more :: tokens
        while (stream.current().type == TokenType::SCOPE)
        {
            stream.advance(); // consume '::'

            if (stream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", stream.location());
            }

            parts.push_back(stream.current().stringValue.getString());
            stream.advance();
        }

        return parts;
    }
}
