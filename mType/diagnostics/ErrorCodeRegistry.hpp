#pragma once
#include "ErrorCode.hpp"

// =============================================================================
// MTYPE_ERROR_CODES — the master table of every diagnostic code in the project.
//
// Format:  X(name, id, severity, title)
//
//   name      Identifier exposed under `diagnostics::codes::<name>`.
//   id        Stable string id printed by the renderer (e.g. "MT-E1001").
//   severity  diagnostics::Severity::Error / Warning / Note / Help.
//   title     Short headline used by the renderer when the converter does
//             not override the message.
//
// Numbering convention (append-only — never reuse a number):
//   MT-E0xxx  parse / syntax
//   MT-E1xxx  name resolution
//   MT-E2xxx  type system
//   MT-E3xxx  access / visibility
//   MT-E4xxx  inheritance / classes
//   MT-E5xxx  runtime
//   MT-E6xxx  environment / file / argument
//   MT-E7xxx  imports / dependencies
//   MT-E9xxx  internal / catch-all
//   MT-W2xxx  warnings
// =============================================================================

#define MTYPE_ERROR_CODES(X) \
    /* ----- parse / syntax ----- */ \
    X(ParseUnexpectedToken,         "MT-E0001", ::diagnostics::Severity::Error, "unexpected token") \
    X(ParseMissingSemicolon,        "MT-E0002", ::diagnostics::Severity::Error, "missing semicolon") \
    X(ParseDuplicateDeclaration,    "MT-E0003", ::diagnostics::Severity::Error, "duplicate declaration") \
    X(ParseDuplicateSignature,      "MT-E0004", ::diagnostics::Severity::Error, "duplicate method signature") \
    X(ParseInvalidSyntax,           "MT-E0005", ::diagnostics::Severity::Error, "invalid syntax") \
    X(ParseInvalidAssignmentTarget, "MT-E0006", ::diagnostics::Severity::Error, "invalid assignment target") \
    X(ParsePrimitiveInGeneric,      "MT-E0007", ::diagnostics::Severity::Error, "primitive type in generic argument") \
    /* ----- name resolution ----- */ \
    X(NameUndefinedVariable,        "MT-E1001", ::diagnostics::Severity::Error, "use of undeclared identifier") \
    X(NameUndefinedFunction,        "MT-E1002", ::diagnostics::Severity::Error, "call to undefined function") \
    X(NameUndefinedClass,           "MT-E1003", ::diagnostics::Severity::Error, "use of undeclared type") \
    X(NameUndefinedMethod,          "MT-E1004", ::diagnostics::Severity::Error, "call to undefined method") \
    X(NameUndefinedField,           "MT-E1005", ::diagnostics::Severity::Error, "use of undeclared field") \
    X(NameClassNotFound,            "MT-E1006", ::diagnostics::Severity::Error, "class not found at runtime") \
    X(NameMethodNotFound,           "MT-E1007", ::diagnostics::Severity::Error, "method not found at runtime") \
    X(NameFieldNotFound,            "MT-E1008", ::diagnostics::Severity::Error, "field not found at runtime") \
    X(NameConstructorNotFound,      "MT-E1009", ::diagnostics::Severity::Error, "constructor not found at runtime") \
    X(NameFunctionNotFound,         "MT-E1010", ::diagnostics::Severity::Error, "function not found at runtime") \
    /* ----- type system ----- */ \
    X(TypeMismatch,                 "MT-E2001", ::diagnostics::Severity::Error, "type mismatch") \
    X(TypeConversionFailed,         "MT-E2002", ::diagnostics::Severity::Error, "type conversion failed") \
    X(TypeResolutionFailed,         "MT-E2003", ::diagnostics::Severity::Error, "could not resolve type") \
    X(TypeAmbiguousCall,            "MT-E2004", ::diagnostics::Severity::Error, "ambiguous overload resolution") \
    X(TypeNoMatchingOverload,       "MT-E2005", ::diagnostics::Severity::Error, "no matching overload") \
    X(TypeInvalidArgument,          "MT-E2006", ::diagnostics::Severity::Error, "invalid argument") \
    X(TypeGeneric,                  "MT-E2007", ::diagnostics::Severity::Error, "type error") \
    /* ----- access / visibility ----- */ \
    X(AccessViolation,              "MT-E3001", ::diagnostics::Severity::Error, "access violation") \
    X(FinalModification,            "MT-E3002", ::diagnostics::Severity::Error, "cannot modify final member") \
    /* ----- inheritance / classes ----- */ \
    X(InheritanceError,             "MT-E4001", ::diagnostics::Severity::Error, "inheritance error") \
    X(AbstractClassInstantiation,   "MT-E4002", ::diagnostics::Severity::Error, "cannot instantiate abstract class") \
    /* ----- runtime ----- */ \
    X(RuntimeNullPointer,           "MT-E5001", ::diagnostics::Severity::Error, "null pointer dereference") \
    X(RuntimeArrayCreation,         "MT-E5002", ::diagnostics::Severity::Error, "invalid array creation") \
    X(RuntimeUserException,         "MT-E5003", ::diagnostics::Severity::Error, "uncaught exception") \
    X(RuntimeObject,                "MT-E5004", ::diagnostics::Severity::Error, "object operation failed") \
    X(RuntimeGeneric,               "MT-E5005", ::diagnostics::Severity::Error, "runtime error") \
    X(RuntimeDivisionByZero,        "MT-E5006", ::diagnostics::Severity::Error, "division by zero") \
    X(RuntimeStackUnderflow,        "MT-E5007", ::diagnostics::Severity::Error, "stack underflow") \
    /* ----- environment / file / argument ----- */ \
    X(EnvironmentError,             "MT-E6001", ::diagnostics::Severity::Error, "environment error") \
    X(FileError,                    "MT-E6002", ::diagnostics::Severity::Error, "file error") \
    X(ArgumentError,                "MT-E6003", ::diagnostics::Severity::Error, "argument error") \
    /* ----- imports / dependencies ----- */ \
    X(ImportCircularDependency,     "MT-E7001", ::diagnostics::Severity::Error, "circular dependency") \
    X(ImportSymbolNotFound,         "MT-E7002", ::diagnostics::Severity::Error, "imported symbol not found") \
    /* ----- internal / catch-all ----- */ \
    X(InternalError,                "MT-E9000", ::diagnostics::Severity::Error, "internal compiler error") \
    /* ----- warnings ----- */ \
    X(UnusedVariable,               "MT-W2001", ::diagnostics::Severity::Warning, "unused variable") \
    X(MissingOverride,              "MT-W2002", ::diagnostics::Severity::Warning, "missing override keyword")

namespace diagnostics::codes
{
#define MTYPE_ERROR_CODES_DECLARE(name, id, sev, title) extern const ::diagnostics::ErrorCode name;
    MTYPE_ERROR_CODES(MTYPE_ERROR_CODES_DECLARE)
#undef MTYPE_ERROR_CODES_DECLARE
}
