#include "ExceptionConverter_Internal.hpp"

namespace diagnostics
{
    namespace detail
    {
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

        Diagnostic convertMissingSemicolon(const errors::MissingSemicolonException& e)
        {
            // Structured "Insert ';'" suggestion. The LSP code-action handler
            // picks it up via the existing data["suggestions"] route — no
            // special-case dispatch in CodeActionHandler is required.
            Suggestion fix;
            fix.label = "insert ';'";
            fix.renderedHint = "help: insert ';' here";
            // Zero-width edit at the missing-semicolon column.
            fix.edits.push_back(TextEdit{
                e.getLocation(), e.getLocation(), ";"
            });
            fix.applicability = FixApplicability::MachineApplicable;

            return DiagnosticBuilder(codes::ParseMissingSemicolon)
                .withMessage("expected ';' at end of statement")
                .withPrimary(e.getLocation(), "expected ';' here")
                .withSuggestion(std::move(fix))
                .withSourceException("MissingSemicolonException")
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
    } // namespace detail

    Diagnostic fromException(const std::exception& e)
    {
        using namespace detail;

        // Most-derived classes first; never reorder unless you know the
        // hierarchy. dynamic_cast against a base would shadow a derived
        // converter and produce the wrong code.

        // ParseException subclasses
        if (auto p = dynamic_cast<const errors::DuplicateDeclarationException*>(&e))
            return convertDuplicateDeclaration(*p);
        if (auto p = dynamic_cast<const errors::DuplicateSignatureException*>(&e))
            return convertDuplicateSignature(*p);
        if (auto p = dynamic_cast<const errors::MissingSemicolonException*>(&e))
            return convertMissingSemicolon(*p);

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
        // MYT-46 typed promotions
        if (auto p = dynamic_cast<const errors::DivisionByZeroException*>(&e))
            return convertDivisionByZero(*p);
        if (auto p = dynamic_cast<const errors::StackUnderflowException*>(&e))
            return convertStackUnderflow(*p);
        if (auto p = dynamic_cast<const errors::ConstructorNotFoundException*>(&e))
            return convertConstructorNotFound(*p);
        if (auto p = dynamic_cast<const errors::FunctionNotFoundException*>(&e))
            return convertFunctionNotFound(*p);

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
