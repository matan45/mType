#pragma once
#include <optional>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/SourceLocation.hpp"

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
    };
}
