#include "ExceptionConverter_Internal.hpp"

namespace diagnostics::detail
{
    Diagnostic convertTypeResolution(const errors::TypeResolutionException& e)
    {
        return plainFromScript(e, codes::TypeResolutionFailed, "TypeResolutionException");
    }

    Diagnostic convertClassNotFound(const errors::ClassNotFoundException& e)
    {
        DiagnosticBuilder b(codes::NameClassNotFound);
        b.withMessage("cannot find class '" + e.getClassName() + "'")
         .withPrimary(e.getLocation(), "not found in this scope")
         .withStackTrace(e.getStackTrace())
         .withSourceException("ClassNotFoundException");
        attachDidYouMean(b, e.getClassName(), e.getIdentifierPool(), "class");
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
         .withStackTrace(e.getStackTrace())
         .withSourceException("MethodNotFoundException");
        attachDidYouMean(b, e.getMethodName(), e.getIdentifierPool(), "method");
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
         .withStackTrace(e.getStackTrace())
         .withSourceException("FieldNotFoundException");
        attachDidYouMean(b, e.getFieldName(), e.getIdentifierPool(), "field");
        return std::move(b).build();
    }

    Diagnostic convertNullPointer(const errors::NullPointerException& e)
    {
        DiagnosticBuilder b(codes::RuntimeNullPointer);
        b.withMessage(stripRuntimePrefix(e.getMessage()))
         .withPrimary(e.getLocation(), "null dereference here")
         .withStackTrace(e.getStackTrace())
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

    Diagnostic convertDivisionByZero(const errors::DivisionByZeroException& e)
    {
        DiagnosticBuilder b(codes::RuntimeDivisionByZero);
        b.withMessage(stripRuntimePrefix(e.getMessage()))
         .withPrimary(e.getLocation(), "divisor is zero")
         .withStackTrace(e.getStackTrace())
         .withSourceException("DivisionByZeroException");
        if (!e.getOperation().empty())
        {
            b.withNote("operation: " + e.getOperation());
        }
        return std::move(b).build();
    }

    Diagnostic convertStackUnderflow(const errors::StackUnderflowException& e)
    {
        DiagnosticBuilder b(codes::RuntimeStackUnderflow);
        b.withMessage(stripRuntimePrefix(e.getMessage()))
         .withPrimary(e.getLocation(), "stack too shallow")
         .withStackTrace(e.getStackTrace())
         .withNote("required " + std::to_string(e.getRequired())
                   + ", available " + std::to_string(e.getAvailable()))
         .withSourceException("StackUnderflowException");
        return std::move(b).build();
    }

    Diagnostic convertConstructorNotFound(const errors::ConstructorNotFoundException& e)
    {
        DiagnosticBuilder b(codes::NameConstructorNotFound);
        b.withMessage("no matching overload for call to constructor of '" + e.getClassName()
                      + "' taking " + std::to_string(e.getParamCount())
                      + " argument(s)")
         .withPrimary(e.getLocation(), "no matching constructor")
         .withStackTrace(e.getStackTrace())
         .withSourceException("ConstructorNotFoundException");
        return std::move(b).build();
    }

    Diagnostic convertFunctionNotFound(const errors::FunctionNotFoundException& e)
    {
        DiagnosticBuilder b(codes::NameFunctionNotFound);
        b.withMessage("cannot find function '" + e.getFunctionName() + "'")
         .withPrimary(e.getLocation(), "not found")
         .withStackTrace(e.getStackTrace())
         .withSourceException("FunctionNotFoundException");
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

    Diagnostic convertParse(const errors::ParseException& e)
    {
        // MYT-360 — primitive-type-in-generic-argument has its own code so
        // the LSP can offer a quick-fix that rewrites `<int>` → `<Int>` and
        // adds the boxed-type import. The message shape is stable:
        // "Primitive type 'int' cannot be used as generic type argument.
        //  Use boxed type 'Int' instead".
        const std::string& msg = e.getMessage();
        if (msg.find("cannot be used as generic type argument") != std::string::npos
            && msg.find("Primitive type") != std::string::npos)
        {
            return plainFromScript(e, codes::ParsePrimitiveInGeneric,
                                   "PrimitiveInGenericException");
        }
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
            .withStackTrace(e.getStackTrace())
            .withSourceException("RuntimeException")
            .build();
    }

    Diagnostic convertUndefined(const errors::UndefinedException& e)
    {
        DiagnosticBuilder b(codes::NameUndefinedVariable);
        b.withMessage(e.getMessage())
         .withPrimary(e.getLocation())
         .withSourceException("UndefinedException");

        // The exception's message often contains the failing identifier in
        // quotes (e.g. "Undefined variable 'foa'"). Pull it out for the
        // suggestion lookup. Fall back to the raw message if no quotes —
        // findClosestMatch rejects quickly when too long for its budget.
        const std::string& msg = e.getMessage();
        std::string query;
        const size_t quoteStart = msg.find('\'');
        if (quoteStart != std::string::npos)
        {
            const size_t quoteEnd = msg.find('\'', quoteStart + 1);
            if (quoteEnd != std::string::npos)
            {
                query = msg.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }
        if (!query.empty())
        {
            attachDidYouMean(b, query, e.getIdentifierPool(), "identifier");
        }
        return std::move(b).build();
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
}
