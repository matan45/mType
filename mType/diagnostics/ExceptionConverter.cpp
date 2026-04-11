#include "ExceptionConverter.hpp"

#include "DiagnosticBuilder.hpp"
#include "ErrorCodeRegistry.hpp"

#include "../errors/AbstractClassException.hpp"
#include "../errors/AccessViolationException.hpp"
#include "../errors/AmbiguousCallException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../errors/ArrayCreationException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"
#include "../errors/DuplicateSignatureException.hpp"
#include "../errors/EnvironmentException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FileException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/InheritanceException.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/NoMatchingOverloadException.hpp"
#include "../errors/NullPointerException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../errors/ScriptException.hpp"
#include "../errors/TypeConversionException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/TypeResolutionException.hpp"
#include "../errors/UndefinedException.hpp"
#ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION
    #include "../errors/UserException.hpp"
#endif
#include "../ast/AccessModifier.hpp"

#include <sstream>
#include <string>

namespace diagnostics
{
    namespace
    {
        // ----- helpers ----------------------------------------------------

        // Many RuntimeException subclasses prepend "Runtime error: " in
        // their constructor. The renderer already brands the diagnostic as
        // an error, so strip the prefix to keep messages tidy.
        std::string stripRuntimePrefix(std::string msg)
        {
            const std::string prefix = "Runtime error: ";
            if (msg.rfind(prefix, 0) == 0)
            {
                return msg.substr(prefix.size());
            }
            return msg;
        }

        // Format an access modifier for diagnostic notes without forcing
        // a link dependency on mtype-ast. Mirrors ast::accessModifierToString.
        const char* accessLevelName(ast::AccessModifier level)
        {
            switch (level)
            {
            case ast::AccessModifier::PRIVATE:   return "private";
            case ast::AccessModifier::PROTECTED: return "protected";
            case ast::AccessModifier::PUBLIC:    return "public";
            }
            return "unknown";
        }

        // Build a diagnostic from a "plain" ScriptException — just the
        // location and the existing message — tagged with the given code.
        Diagnostic plainFromScript(const errors::ScriptException& e,
                                    const ErrorCode& code,
                                    const char* exceptionTypeName)
        {
            return DiagnosticBuilder(code)
                .withMessage(e.getMessage())
                .withPrimary(e.getLocation())
                .withSourceException(exceptionTypeName)
                .build();
        }

        // ----- specific converters ----------------------------------------

        Diagnostic convertDuplicateDeclaration(const errors::DuplicateDeclarationException& e)
        {
            const std::string article =
                (e.getDeclarationType() == "interface") ? "an" : "a";
            const std::string headline = "duplicate " + e.getDeclarationType()
                + " declaration: '" + e.getDuplicateName() + "'";

            DiagnosticBuilder b(codes::ParseDuplicateDeclaration);
            b.withMessage(headline)
             .withPrimary(e.getSecondDeclaration(),
                          "redeclared here")
             .withSecondary(e.getFirstDeclaration(),
                            "first declared here as " + article + " "
                            + e.getDeclarationType())
             .withSourceException("DuplicateDeclarationException");
            return std::move(b).build();
        }

        Diagnostic convertDuplicateSignature(const errors::DuplicateSignatureException& e)
        {
            const std::string headline = "duplicate " + e.getDeclarationType()
                + " signature: '" + e.getMethodName()
                + "(" + e.getSignature() + ")'";

            return DiagnosticBuilder(codes::ParseDuplicateSignature)
                .withMessage(headline)
                .withPrimary(e.getSecondDeclaration(), "redeclared here")
                .withSecondary(e.getFirstDeclaration(), "first declared here")
                .withSourceException("DuplicateSignatureException")
                .build();
        }

        Diagnostic convertAccessViolation(const errors::AccessViolationException& e)
        {
            DiagnosticBuilder b(codes::AccessViolation);
            b.withMessage(e.getMessage())
             .withPrimary(e.getLocation())
             .withSourceException("AccessViolationException");

            if (!e.getMemberName().empty() && !e.getTargetClassName().empty())
            {
                std::ostringstream note;
                note << e.getMemberType() << " '" << e.getMemberName()
                     << "' is " << accessLevelName(e.getAccessLevel())
                     << " in class '" << e.getTargetClassName() << "'";
                b.withNote(note.str());
            }
            if (e.getAccessLevel() == ast::AccessModifier::PROTECTED
                && !e.getTargetClassName().empty())
            {
                b.withNote("protected members are accessible from '"
                            + e.getTargetClassName() + "' and its subclasses");
            }
            else if (e.getAccessLevel() == ast::AccessModifier::PRIVATE)
            {
                b.withNote("consider making the member public or adding a public accessor");
            }
            return std::move(b).build();
        }

        Diagnostic convertAmbiguousCall(const errors::AmbiguousCallException& e)
        {
            DiagnosticBuilder b(codes::TypeAmbiguousCall);
            b.withMessage("ambiguous call to '" + e.getMethodName() + "'")
             .withPrimary(e.getLocation())
             .withSourceException("AmbiguousCallException");

            // The message already enumerates candidates; surface them as
            // structured notes so the renderer can color/indent them.
            const auto& candidates = e.getCandidateSignatures();
            for (size_t i = 0; i < candidates.size(); ++i)
            {
                b.withNote("candidate " + std::to_string(i + 1) + ": "
                            + e.getMethodName() + "(" + candidates[i] + ")");
            }
            b.withNote("provide an explicit cast to disambiguate");
            return std::move(b).build();
        }

        Diagnostic convertNoMatchingOverload(const errors::NoMatchingOverloadException& e)
        {
            DiagnosticBuilder b(codes::TypeNoMatchingOverload);
            b.withMessage("no matching overload for call to '" + e.getMethodName() + "'")
             .withPrimary(e.getLocation())
             .withSourceException("NoMatchingOverloadException");

            const auto& available = e.getAvailableSignatures();
            if (available.empty())
            {
                b.withNote("no overloads of '" + e.getMethodName() + "' are defined");
            }
            else
            {
                for (size_t i = 0; i < available.size(); ++i)
                {
                    b.withNote("available " + std::to_string(i + 1) + ": "
                                + e.getMethodName() + "(" + available[i] + ")");
                }
            }
            return std::move(b).build();
        }

        Diagnostic convertTypeConversion(const errors::TypeConversionException& e)
        {
            DiagnosticBuilder b(codes::TypeConversionFailed);
            b.withMessage("cannot convert '" + e.getSourceType()
                         + "' to '" + e.getTargetType() + "'")
             .withPrimary(e.getLocation())
             .withSourceException("TypeConversionException");
            return std::move(b).build();
        }

        Diagnostic convertTypeResolution(const errors::TypeResolutionException& e)
        {
            return plainFromScript(e, codes::TypeResolutionFailed, "TypeResolutionException");
        }

        Diagnostic convertClassNotFound(const errors::ClassNotFoundException& e)
        {
            DiagnosticBuilder b(codes::NameClassNotFound);
            b.withMessage("cannot find class '" + e.getClassName() + "'")
             .withPrimary(e.getLocation(), "not found in this scope")
             .withSourceException("ClassNotFoundException");
            return std::move(b).build();
        }

        Diagnostic convertMethodNotFound(const errors::MethodNotFoundException& e)
        {
            DiagnosticBuilder b(codes::NameMethodNotFound);
            const std::string headline = e.getClassName().empty()
                ? "cannot find method '" + e.getMethodName() + "'"
                : "cannot find method '" + e.getMethodName()
                  + "' on class '" + e.getClassName() + "'";
            b.withMessage(headline)
             .withPrimary(e.getLocation(), "not found")
             .withSourceException("MethodNotFoundException");
            return std::move(b).build();
        }

        Diagnostic convertFieldNotFound(const errors::FieldNotFoundException& e)
        {
            DiagnosticBuilder b(codes::NameFieldNotFound);
            const std::string headline = e.getClassName().empty()
                ? "cannot find field '" + e.getFieldName() + "'"
                : "cannot find field '" + e.getFieldName()
                  + "' on class '" + e.getClassName() + "'";
            b.withMessage(headline)
             .withPrimary(e.getLocation(), "not found")
             .withSourceException("FieldNotFoundException");
            return std::move(b).build();
        }

        Diagnostic convertNullPointer(const errors::NullPointerException& e)
        {
            DiagnosticBuilder b(codes::RuntimeNullPointer);
            b.withMessage(stripRuntimePrefix(e.getMessage()))
             .withPrimary(e.getLocation(), "null dereference here")
             .withSourceException("NullPointerException");
            if (!e.getOperation().empty())
            {
                b.withNote("during " + e.getOperation());
            }
            return std::move(b).build();
        }

        Diagnostic convertArrayCreation(const errors::ArrayCreationException& e)
        {
            DiagnosticBuilder b(codes::RuntimeArrayCreation);
            b.withMessage(stripRuntimePrefix(e.getMessage()))
             .withPrimary(e.getLocation())
             .withSourceException("ArrayCreationException");
            if (!e.getArrayType().empty())
            {
                b.withNote("array element type: " + e.getArrayType());
            }
            return std::move(b).build();
        }

        Diagnostic convertFinalModification(const errors::FinalModificationException& e)
        {
            DiagnosticBuilder b(codes::FinalModification);
            const std::string headline = e.getClassName().empty()
                ? "cannot modify final member '" + e.getMemberName() + "'"
                : "cannot modify final member '" + e.getMemberName()
                  + "' on class '" + e.getClassName() + "'";
            b.withMessage(headline)
             .withPrimary(e.getLocation(), "this assignment is forbidden")
             .withNote("`final` members can only be assigned once, in their declaration or constructor")
             .withSourceException("FinalModificationException");
            return std::move(b).build();
        }

        Diagnostic convertObject(const errors::ObjectException& e)
        {
            DiagnosticBuilder b(codes::RuntimeObject);
            b.withMessage(stripRuntimePrefix(e.getMessage()))
             .withPrimary(e.getLocation())
             .withSourceException("ObjectException");
            if (!e.getObjectType().empty())
            {
                b.withNote("object type: " + e.getObjectType());
            }
            return std::move(b).build();
        }

        Diagnostic convertAbstract(const errors::AbstractClassException& e)
        {
            DiagnosticBuilder b(codes::AbstractClassInstantiation);
            b.withMessage(e.getMessage())
             .withPrimary(e.getLocation())
             .withSourceException("AbstractClassException");
            if (!e.getClassName().empty())
            {
                b.withNote("class: " + e.getClassName());
            }
            for (const auto& m : e.getUnimplementedMethods())
            {
                b.withNote("missing implementation: " + m);
            }
            return std::move(b).build();
        }

        Diagnostic convertInheritance(const errors::InheritanceException& e)
        {
            DiagnosticBuilder b(codes::InheritanceError);
            b.withMessage(e.getMessage())
             .withPrimary(e.getLocation())
             .withSourceException("InheritanceException");
            if (!e.getChildClassName().empty() && !e.getParentClassName().empty())
            {
                b.withNote("between child '" + e.getChildClassName()
                            + "' and parent '" + e.getParentClassName() + "'");
            }
            if (!e.getMethodName().empty())
            {
                b.withNote("method: " + e.getMethodName());
            }
            return std::move(b).build();
        }

#ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION
        Diagnostic convertUserException(const errors::UserException& e)
        {
            DiagnosticBuilder b(codes::RuntimeUserException);
            b.withMessage("uncaught exception of type '"
                         + e.getExceptionTypeName() + "'")
             .withPrimary(e.getLocation(), "thrown here")
             .withStackTrace(e.getStackTrace())
             .withSourceException("UserException");
            return std::move(b).build();
        }
#endif

        // ----- mid-tier base classes --------------------------------------

        Diagnostic convertParse(const errors::ParseException& e)
        {
            return plainFromScript(e, codes::ParseInvalidSyntax, "ParseException");
        }

        Diagnostic convertType(const errors::TypeException& e)
        {
            return plainFromScript(e, codes::TypeGeneric, "TypeException");
        }

        Diagnostic convertRuntime(const errors::RuntimeException& e)
        {
            return DiagnosticBuilder(codes::RuntimeGeneric)
                .withMessage(stripRuntimePrefix(e.getMessage()))
                .withPrimary(e.getLocation())
                .withSourceException("RuntimeException")
                .build();
        }

        Diagnostic convertUndefined(const errors::UndefinedException& e)
        {
            return plainFromScript(e, codes::NameUndefinedVariable, "UndefinedException");
        }

        Diagnostic convertEnvironment(const errors::EnvironmentException& e)
        {
            return plainFromScript(e, codes::EnvironmentError, "EnvironmentException");
        }

        Diagnostic convertFile(const errors::FileException& e)
        {
            return plainFromScript(e, codes::FileError, "FileException");
        }

        Diagnostic convertArgument(const errors::ArgumentException& e)
        {
            return plainFromScript(e, codes::ArgumentError, "ArgumentException");
        }

        Diagnostic convertScript(const errors::ScriptException& e)
        {
            return plainFromScript(e, codes::InternalError, "ScriptException");
        }

        Diagnostic convertUnknown(const std::exception& e)
        {
            return DiagnosticBuilder(codes::InternalError)
                .withMessage(e.what())
                .withSourceException("std::exception")
                .build();
        }
    } // namespace

    Diagnostic fromException(const std::exception& e)
    {
        // Most-derived classes first; never reorder unless you know the
        // hierarchy. dynamic_cast against a base would shadow a derived
        // converter and produce the wrong code.

        // ParseException subclasses
        if (auto p = dynamic_cast<const errors::DuplicateDeclarationException*>(&e))
            return convertDuplicateDeclaration(*p);
        if (auto p = dynamic_cast<const errors::DuplicateSignatureException*>(&e))
            return convertDuplicateSignature(*p);

        // TypeException subclasses
        if (auto p = dynamic_cast<const errors::AccessViolationException*>(&e))
            return convertAccessViolation(*p);
        if (auto p = dynamic_cast<const errors::AmbiguousCallException*>(&e))
            return convertAmbiguousCall(*p);
        if (auto p = dynamic_cast<const errors::NoMatchingOverloadException*>(&e))
            return convertNoMatchingOverload(*p);
        if (auto p = dynamic_cast<const errors::TypeConversionException*>(&e))
            return convertTypeConversion(*p);
        if (auto p = dynamic_cast<const errors::TypeResolutionException*>(&e))
            return convertTypeResolution(*p);

        // RuntimeException subclasses
        if (auto p = dynamic_cast<const errors::ClassNotFoundException*>(&e))
            return convertClassNotFound(*p);
        if (auto p = dynamic_cast<const errors::MethodNotFoundException*>(&e))
            return convertMethodNotFound(*p);
        if (auto p = dynamic_cast<const errors::FieldNotFoundException*>(&e))
            return convertFieldNotFound(*p);
        if (auto p = dynamic_cast<const errors::NullPointerException*>(&e))
            return convertNullPointer(*p);
        if (auto p = dynamic_cast<const errors::ArrayCreationException*>(&e))
            return convertArrayCreation(*p);
        if (auto p = dynamic_cast<const errors::FinalModificationException*>(&e))
            return convertFinalModification(*p);
        if (auto p = dynamic_cast<const errors::ObjectException*>(&e))
            return convertObject(*p);

        // ScriptException subclasses (mid-tier siblings)
        if (auto p = dynamic_cast<const errors::AbstractClassException*>(&e))
            return convertAbstract(*p);
        if (auto p = dynamic_cast<const errors::InheritanceException*>(&e))
            return convertInheritance(*p);
#ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION
        if (auto p = dynamic_cast<const errors::UserException*>(&e))
            return convertUserException(*p);
#endif
        if (auto p = dynamic_cast<const errors::UndefinedException*>(&e))
            return convertUndefined(*p);
        if (auto p = dynamic_cast<const errors::EnvironmentException*>(&e))
            return convertEnvironment(*p);
        if (auto p = dynamic_cast<const errors::FileException*>(&e))
            return convertFile(*p);
        if (auto p = dynamic_cast<const errors::ArgumentException*>(&e))
            return convertArgument(*p);

        // Mid-tier base classes
        if (auto p = dynamic_cast<const errors::ParseException*>(&e))
            return convertParse(*p);
        if (auto p = dynamic_cast<const errors::TypeException*>(&e))
            return convertType(*p);
        if (auto p = dynamic_cast<const errors::RuntimeException*>(&e))
            return convertRuntime(*p);

        // Root
        if (auto p = dynamic_cast<const errors::ScriptException*>(&e))
            return convertScript(*p);

        // Anything else (std::runtime_error, etc.)
        return convertUnknown(e);
    }
}
