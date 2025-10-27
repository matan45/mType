#pragma once

#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <vector>
#include <memory>

namespace parser::utilities
{
    using namespace ast::nodes::annotations;
    using namespace token;
    using namespace errors;

    /**
     * @brief Utility class for parsing annotations from token streams
     *
     * Provides centralized logic for detecting and parsing @AnnotationName syntax
     * in class and method definitions.
     */
    class AnnotationParser
    {
    public:
        /**
         * @brief Parse a single annotation at current token position
         *
         * Expects current token to be '@', followed by an identifier.
         * Returns nullptr if current token is not '@'.
         *
         * @param tokenStream The token stream to parse from
         * @return Shared pointer to AnnotationNode, or nullptr if no annotation present
         */
        static std::shared_ptr<AnnotationNode> parseAnnotation(TokenStream& tokenStream);

        /**
         * @brief Parse all consecutive annotations at current position
         *
         * Continues parsing annotations while the current token is '@'.
         * Returns empty vector if no annotations are present.
         *
         * @param tokenStream The token stream to parse from
         * @return Vector of AnnotationNode pointers (may be empty)
         */
        static std::vector<std::shared_ptr<AnnotationNode>> parseAnnotations(TokenStream& tokenStream);

        /**
         * @brief Check if current token is the start of an annotation (@)
         *
         * @param type The token type to check
         * @return true if token is AT (@)
         */
        static bool isAnnotation(TokenType type);
    };
}
