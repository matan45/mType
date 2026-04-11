#pragma once
#include <string>
#include <utility>
#include <vector>
#include "Diagnostic.hpp"
#include "ErrorCode.hpp"

namespace diagnostics
{
    /**
     * Fluent builder for Diagnostic. Used by ExceptionConverter to keep
     * per-exception conversion functions short and readable.
     *
     * Example:
     *   return DiagnosticBuilder(codes::NameUndefinedVariable)
     *       .withMessage("cannot find value 'foa' in this scope")
     *       .withPrimary(loc, "not found in this scope")
     *       .withNote("a local variable with a similar name exists")
     *       .withSourceException("UndefinedException")
     *       .build();
     */
    class DiagnosticBuilder
    {
    public:
        explicit DiagnosticBuilder(const ErrorCode& code)
        {
            diag_.code = &code;
            diag_.severity = code.defaultSeverity;
            diag_.message = std::string(code.title);
        }

        DiagnosticBuilder& withSeverity(Severity sev)
        {
            diag_.severity = sev;
            return *this;
        }

        DiagnosticBuilder& withMessage(std::string message)
        {
            diag_.message = std::move(message);
            return *this;
        }

        DiagnosticBuilder& withPrimary(const errors::SourceLocation& loc, std::string label = "")
        {
            diag_.primarySpan = LabeledSpan(loc);
            diag_.primarySpan.label = std::move(label);
            diag_.primarySpan.isPrimary = true;
            return *this;
        }

        DiagnosticBuilder& withPrimaryRange(const errors::SourceLocation& start,
                                             const errors::SourceLocation& end,
                                             std::string label = "")
        {
            diag_.primarySpan = LabeledSpan(start, end);
            diag_.primarySpan.label = std::move(label);
            diag_.primarySpan.isPrimary = true;
            return *this;
        }

        DiagnosticBuilder& withSecondary(const errors::SourceLocation& loc, std::string label = "")
        {
            LabeledSpan span(loc);
            span.label = std::move(label);
            span.isPrimary = false;
            diag_.secondarySpans.push_back(std::move(span));
            return *this;
        }

        DiagnosticBuilder& withNote(std::string note)
        {
            diag_.notes.push_back(std::move(note));
            return *this;
        }

        DiagnosticBuilder& withSuggestion(Suggestion suggestion)
        {
            diag_.suggestions.push_back(std::move(suggestion));
            return *this;
        }

        DiagnosticBuilder& withStackTrace(std::vector<std::string> frames)
        {
            diag_.stackTrace = std::move(frames);
            return *this;
        }

        DiagnosticBuilder& withSourceException(std::string typeName)
        {
            diag_.sourceExceptionType = std::move(typeName);
            return *this;
        }

        Diagnostic build() &&
        {
            return std::move(diag_);
        }

        Diagnostic build() const &
        {
            return diag_;
        }

    private:
        Diagnostic diag_;
    };
}
