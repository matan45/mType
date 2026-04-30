#pragma once
#include <optional>
#include <string>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/SourceLocation.hpp"

namespace environment { class Environment; }

namespace vm::runtime::utils
{
    /**
     * Utility class for attaching source location information to runtime errors
     * Eliminates duplication of getSourceLocation + conditional error throwing pattern
     */
    class ErrorLocationHelper
    {
    public:
        /**
         * Throws RuntimeException with source location if available
         * @param context Execution context containing program and instruction pointer
         * @param message Error message to include in the exception
         * @throws errors::RuntimeException with or without source location
         */
        [[noreturn]] static void throwRuntimeError(const ExecutionContext& context, const std::string& message)
        {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw errors::RuntimeException(message, errorLoc);
            } else {
                throw errors::RuntimeException(message);
            }
        }

        /**
         * Gets source location from execution context if available
         * @param context Execution context containing program and instruction pointer
         * @return Optional SourceLocation, empty if not found
         */
        static std::optional<errors::SourceLocation> getSourceLocation(const ExecutionContext& context)
        {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                return errors::SourceLocation(loc->filename, loc->line, loc->column);
            }
            return std::nullopt;
        }

        /**
         * Throws a custom exception type with source location if available
         * @tparam ExceptionType Must be constructible with (message, SourceLocation)
         * @param context Execution context containing program and instruction pointer
         * @param message Error message to include in the exception
         * @throws ExceptionType with or without source location
         */
        template<typename ExceptionType>
        [[noreturn]] static void throwError(const ExecutionContext& context, const std::string& message)
        {
            auto* loc = context.program->getSourceLocation(context.instructionPointer);
            if (loc) {
                errors::SourceLocation errorLoc(loc->filename, loc->line, loc->column);
                throw ExceptionType(message, errorLoc);
            } else {
                throw ExceptionType(message);
            }
        }

        /**
         * Construct an instance of the user-level Exception subclass `typeName`
         * (resolved through the running environment's ClassRegistry), populate
         * its `message` field, and throw it as `errors::UserException` so the
         * existing `catch (errors::UserException&)` dispatch in the VM loop
         * routes it through the function's ExceptionTable. If `typeName` is
         * not loaded, falls back through "RuntimeException" -> "Exception" ->
         * plain `errors::RuntimeException` so unimported scripts keep their
         * old abort-on-runtime-error behaviour.
         *
         * Lifts MYT-243 (array bounds), MYT-244 (null receiver, CALL_METHOD
         * non-object), MYT-246 (reflection forName).
         */
        [[noreturn]] static void throwUserException(
            const ExecutionContext& context,
            const std::string& typeName,
            const std::string& message);

        /**
         * Same bridge for native callers that hold an Environment but no
         * ExecutionContext (e.g. reflection / net natives). Source location
         * is left empty — the dispatch loop attaches the catch-site IP.
         */
        [[noreturn]] static void throwUserException(
            const std::shared_ptr<environment::Environment>& env,
            const std::string& typeName,
            const std::string& message);
    };
}
