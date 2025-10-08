#pragma once
#include "../ast/ASTNode.hpp"
#include <memory>
#include <optional>
#include <functional>

namespace parser
{
    class StatementParser;
    class ExpressionParser;
    class ClassParser;
    class InterfaceParser;
    class TokenStream;

    using namespace ast;

    /// @brief Enables inter-parser communication without circular dependencies
    /// Provides clean interface for parser delegation with memory-safe references
    class ParseContext
    {
    private:
        std::optional<std::reference_wrapper<StatementParser>> statementParser;
        std::optional<std::reference_wrapper<ExpressionParser>> expressionParser;
        std::optional<std::reference_wrapper<ClassParser>> classParser;
        std::optional<std::reference_wrapper<InterfaceParser>> interfaceParser;
        std::optional<std::reference_wrapper<TokenStream>> tokenStream;

        // Context flags
        bool insideLambdaBody = false;
        bool insideAsyncFunction = false;  // NEW: Track if we're inside an async function/method

    public:
        /// @brief Default constructor for delayed initialization
        explicit ParseContext() = default;

        /// @brief Constructor with immediate initialization
        explicit ParseContext(StatementParser& stmt, ExpressionParser& expr, ClassParser& cls, InterfaceParser& iface,
                     TokenStream& stream);

        ~ParseContext() = default;

        /// @brief Parse a statement using StatementParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();

        /// @brief Parse an expression using ExpressionParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();

        /// @brief Parse a class using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();

        /// @brief Parse an interface using InterfaceParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseInterface();

        /// @brief Parse a new expression using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();

        // Setters for delayed initialization with memory-safe references
        void setStatementParser(StatementParser& parser);
        void setExpressionParser(ExpressionParser& parser);
        void setClassParser(ClassParser& parser);
        void setInterfaceParser(InterfaceParser& parser);
        void setTokenStream(TokenStream& stream);

        // NEW: Async context management
        /// @brief Check if we're currently inside an async function/method
        [[nodiscard]] bool isInsideAsyncFunction() const { return insideAsyncFunction; }

        /// @brief Set async function context (for entering async function body)
        void setAsyncContext(bool async) { insideAsyncFunction = async; }

        /// @brief RAII helper for async context management
        class AsyncContextGuard {
        private:
            ParseContext& context;
            bool previousState;
        public:
            explicit AsyncContextGuard(ParseContext& ctx, bool async)
                : context(ctx), previousState(ctx.insideAsyncFunction) {
                context.insideAsyncFunction = async;
            }
            ~AsyncContextGuard() {
                context.insideAsyncFunction = previousState;
            }
            // Prevent copying
            AsyncContextGuard(const AsyncContextGuard&) = delete;
            AsyncContextGuard& operator=(const AsyncContextGuard&) = delete;
        };
    };
}
