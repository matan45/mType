#pragma once

#include "../constants/SecurityConstants.hpp"
#include "../errors/ParseException.hpp"
#include <cstddef>
#include <string>

namespace parser
{
    /// @brief Manages parsing context state flags (inside function, class, etc.)
    /// Provides RAII guards for automatic context management
    class ParserContextState
    {
    private:
        // Context flags
        bool insideAsyncFunction = false;
        bool insideFunctionBody = false;
        bool insideClassBody = false;
        bool insideInterfaceBody = false;
        bool insideConstructorBody = false;

        // SECURITY: tracks current expression-parser recursion depth so the
        // parser can reject pathologically nested input (e.g. ((((...x)))))
        // before exhausting the C++ call stack.
        size_t recursionDepth = 0;

    public:
        ParserContextState() = default;
        ~ParserContextState() = default;

        // Recursion depth tracking
        [[nodiscard]] size_t getRecursionDepth() const noexcept { return recursionDepth; }

        /// @brief RAII guard that increments parser recursion depth on
        /// construction and decrements on destruction. Throws ParseException
        /// if the depth would exceed MAX_PARSER_RECURSION_DEPTH.
        class RecursionDepthGuard
        {
        private:
            ParserContextState& context;
        public:
            explicit RecursionDepthGuard(ParserContextState& ctx)
                : context(ctx)
            {
                if (++context.recursionDepth > constants::security::MAX_PARSER_RECURSION_DEPTH)
                {
                    --context.recursionDepth;
                    throw errors::ParseException("Parser recursion depth exceeded maximum of " +
                        std::to_string(constants::security::MAX_PARSER_RECURSION_DEPTH));
                }
            }
            ~RecursionDepthGuard()
            {
                --context.recursionDepth;
            }
            RecursionDepthGuard(const RecursionDepthGuard&) = delete;
            RecursionDepthGuard& operator=(const RecursionDepthGuard&) = delete;
            RecursionDepthGuard(RecursionDepthGuard&&) = delete;
            RecursionDepthGuard& operator=(RecursionDepthGuard&&) = delete;
        };

        // Async context management
        [[nodiscard]] bool isInsideAsyncFunction() const noexcept { return insideAsyncFunction; }
        void setAsyncContext(bool async) noexcept { insideAsyncFunction = async; }

        /// @brief RAII helper for async context management
        class AsyncContextGuard
        {
        private:
            ParserContextState& context;
            bool previousState;
        public:
            explicit AsyncContextGuard(ParserContextState& ctx, bool async)
                : context(ctx), previousState(ctx.insideAsyncFunction)
            {
                context.insideAsyncFunction = async;
            }
            ~AsyncContextGuard()
            {
                context.insideAsyncFunction = previousState;
            }
            // Prevent copying and moving (RAII guard should not be transferred)
            AsyncContextGuard(const AsyncContextGuard&) = delete;
            AsyncContextGuard& operator=(const AsyncContextGuard&) = delete;
            AsyncContextGuard(AsyncContextGuard&&) = delete;
            AsyncContextGuard& operator=(AsyncContextGuard&&) = delete;
        };

        // Function context management
        [[nodiscard]] bool isInsideFunctionBody() const noexcept { return insideFunctionBody; }
        void setFunctionContext(bool inFunction) noexcept { insideFunctionBody = inFunction; }

        /// @brief RAII helper for function context management
        class FunctionContextGuard
        {
        private:
            ParserContextState& context;
            bool previousState;
        public:
            explicit FunctionContextGuard(ParserContextState& ctx)
                : context(ctx), previousState(ctx.insideFunctionBody)
            {
                context.insideFunctionBody = true;
            }
            ~FunctionContextGuard()
            {
                context.insideFunctionBody = previousState;
            }
            // Prevent copying and moving (RAII guard should not be transferred)
            FunctionContextGuard(const FunctionContextGuard&) = delete;
            FunctionContextGuard& operator=(const FunctionContextGuard&) = delete;
            FunctionContextGuard(FunctionContextGuard&&) = delete;
            FunctionContextGuard& operator=(FunctionContextGuard&&) = delete;
        };

        // Class context management
        [[nodiscard]] bool isInsideClassBody() const noexcept { return insideClassBody; }
        void setClassContext(bool inClass) noexcept { insideClassBody = inClass; }

        /// @brief RAII helper for class context management
        class ClassContextGuard
        {
        private:
            ParserContextState& context;
            bool previousState;
        public:
            explicit ClassContextGuard(ParserContextState& ctx)
                : context(ctx), previousState(ctx.insideClassBody)
            {
                context.insideClassBody = true;
            }
            ~ClassContextGuard()
            {
                context.insideClassBody = previousState;
            }
            // Prevent copying and moving (RAII guard should not be transferred)
            ClassContextGuard(const ClassContextGuard&) = delete;
            ClassContextGuard& operator=(const ClassContextGuard&) = delete;
            ClassContextGuard(ClassContextGuard&&) = delete;
            ClassContextGuard& operator=(ClassContextGuard&&) = delete;
        };

        // Interface context management
        [[nodiscard]] bool isInsideInterfaceBody() const noexcept { return insideInterfaceBody; }
        void setInterfaceContext(bool inInterface) noexcept { insideInterfaceBody = inInterface; }

        /// @brief RAII helper for interface context management
        class InterfaceContextGuard
        {
        private:
            ParserContextState& context;
            bool previousState;
        public:
            explicit InterfaceContextGuard(ParserContextState& ctx)
                : context(ctx), previousState(ctx.insideInterfaceBody)
            {
                context.insideInterfaceBody = true;
            }
            ~InterfaceContextGuard()
            {
                context.insideInterfaceBody = previousState;
            }
            // Prevent copying and moving (RAII guard should not be transferred)
            InterfaceContextGuard(const InterfaceContextGuard&) = delete;
            InterfaceContextGuard& operator=(const InterfaceContextGuard&) = delete;
            InterfaceContextGuard(InterfaceContextGuard&&) = delete;
            InterfaceContextGuard& operator=(InterfaceContextGuard&&) = delete;
        };

        // Constructor context management
        [[nodiscard]] bool isInsideConstructorBody() const noexcept { return insideConstructorBody; }
        void setConstructorContext(bool inConstructor) noexcept { insideConstructorBody = inConstructor; }

        /// @brief RAII helper for constructor context management
        class ConstructorContextGuard
        {
        private:
            ParserContextState& context;
            bool previousState;
        public:
            explicit ConstructorContextGuard(ParserContextState& ctx)
                : context(ctx), previousState(ctx.insideConstructorBody)
            {
                context.insideConstructorBody = true;
            }
            ~ConstructorContextGuard()
            {
                context.insideConstructorBody = previousState;
            }
            // Prevent copying and moving (RAII guard should not be transferred)
            ConstructorContextGuard(const ConstructorContextGuard&) = delete;
            ConstructorContextGuard& operator=(const ConstructorContextGuard&) = delete;
            ConstructorContextGuard(ConstructorContextGuard&&) = delete;
            ConstructorContextGuard& operator=(ConstructorContextGuard&&) = delete;
        };

    };
}
