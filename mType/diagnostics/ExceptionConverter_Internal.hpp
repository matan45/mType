#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "DiagnosticBuilder.hpp"
#include "ErrorCodeRegistry.hpp"
#include "ExceptionConverter.hpp"
#include "../ast/AccessModifier.hpp"
#include "../util/DidYouMean.hpp"

#include "../errors/AbstractClassException.hpp"
#include "../errors/AccessViolationException.hpp"
#include "../errors/AmbiguousCallException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../errors/ArrayCreationException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/ConstructorNotFoundException.hpp"
#include "../errors/DivisionByZeroException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"
#include "../errors/DuplicateSignatureException.hpp"
#include "../errors/EnvironmentException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FileException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/FunctionNotFoundException.hpp"
#include "../errors/InheritanceException.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/MissingSemicolonException.hpp"
#include "../errors/NoMatchingOverloadException.hpp"
#include "../errors/NullPointerException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../errors/ScriptException.hpp"
#include "../errors/StackUnderflowException.hpp"
#include "../errors/TypeConversionException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/TypeResolutionException.hpp"
#include "../errors/UndefinedException.hpp"
#ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION
    #include "../errors/UserException.hpp"
#endif

namespace diagnostics::detail
{
    // Many RuntimeException subclasses prepend "Runtime error: " in their
    // constructor. The renderer already brands the diagnostic as an error,
    // so strip the prefix to keep messages tidy.
    inline std::string stripRuntimePrefix(std::string msg)
    {
        const std::string prefix = "Runtime error: ";
        if (msg.rfind(prefix, 0) == 0)
        {
            return msg.substr(prefix.size());
        }
        return msg;
    }

    // Format an access modifier for diagnostic notes without forcing a link
    // dependency on mtype-ast. Mirrors ast::accessModifierToString.
    inline const char* accessLevelName(ast::AccessModifier level)
    {
        switch (level)
        {
        case ast::AccessModifier::PRIVATE:   return "private";
        case ast::AccessModifier::PROTECTED: return "protected";
        case ast::AccessModifier::PUBLIC:    return "public";
        }
        return "unknown";
    }

    // Build a diagnostic from a "plain" ScriptException — just the location
    // and the existing message — tagged with the given code.
    inline Diagnostic plainFromScript(const errors::ScriptException& e,
                                      const ErrorCode& code,
                                      const char* exceptionTypeName)
    {
        return DiagnosticBuilder(code)
            .withMessage(e.getMessage())
            .withPrimary(e.getLocation())
            .withSourceException(exceptionTypeName)
            .build();
    }

    // Try to attach a "did you mean ..." suggestion to a builder. No-op if
    // the pool is empty or no candidate is close enough. The query is the
    // misspelled identifier; the noun ("variable", "class", "method", "field")
    // is folded into the rendered hint.
    inline void attachDidYouMean(DiagnosticBuilder& b,
                                 const std::string& query,
                                 const std::vector<std::string>& pool,
                                 const char* noun)
    {
        if (pool.empty())
        {
            return;
        }
        auto match = util::findClosestMatch(query, pool);
        if (!match)
        {
            return;
        }
        Suggestion s;
        s.label = "did you mean '" + *match + "'?";
        s.renderedHint = "a " + std::string(noun)
                        + " with a similar name exists: '"
                        + *match + "'";
        s.applicability = FixApplicability::MaybeIncorrect;
        b.withSuggestion(std::move(s));
    }

    // Parse-phase + type-check converters (defined in ExceptionConverter.cpp)
    Diagnostic convertDuplicateDeclaration(const errors::DuplicateDeclarationException& e);
    Diagnostic convertDuplicateSignature(const errors::DuplicateSignatureException& e);
    Diagnostic convertMissingSemicolon(const errors::MissingSemicolonException& e);
    Diagnostic convertAccessViolation(const errors::AccessViolationException& e);
    Diagnostic convertAmbiguousCall(const errors::AmbiguousCallException& e);
    Diagnostic convertNoMatchingOverload(const errors::NoMatchingOverloadException& e);
    Diagnostic convertTypeConversion(const errors::TypeConversionException& e);

    // Runtime + name-resolution + mid-tier converters (defined in ExceptionConverter_Messages.cpp)
    Diagnostic convertTypeResolution(const errors::TypeResolutionException& e);
    Diagnostic convertClassNotFound(const errors::ClassNotFoundException& e);
    Diagnostic convertMethodNotFound(const errors::MethodNotFoundException& e);
    Diagnostic convertFieldNotFound(const errors::FieldNotFoundException& e);
    Diagnostic convertNullPointer(const errors::NullPointerException& e);
    Diagnostic convertArrayCreation(const errors::ArrayCreationException& e);
    Diagnostic convertFinalModification(const errors::FinalModificationException& e);
    Diagnostic convertObject(const errors::ObjectException& e);
    Diagnostic convertDivisionByZero(const errors::DivisionByZeroException& e);
    Diagnostic convertStackUnderflow(const errors::StackUnderflowException& e);
    Diagnostic convertConstructorNotFound(const errors::ConstructorNotFoundException& e);
    Diagnostic convertFunctionNotFound(const errors::FunctionNotFoundException& e);
    Diagnostic convertAbstract(const errors::AbstractClassException& e);
    Diagnostic convertInheritance(const errors::InheritanceException& e);
#ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION
    Diagnostic convertUserException(const errors::UserException& e);
#endif
    Diagnostic convertParse(const errors::ParseException& e);
    Diagnostic convertType(const errors::TypeException& e);
    Diagnostic convertRuntime(const errors::RuntimeException& e);
    Diagnostic convertUndefined(const errors::UndefinedException& e);
    Diagnostic convertEnvironment(const errors::EnvironmentException& e);
    Diagnostic convertFile(const errors::FileException& e);
    Diagnostic convertArgument(const errors::ArgumentException& e);
    Diagnostic convertScript(const errors::ScriptException& e);
    Diagnostic convertUnknown(const std::exception& e);
}
