#include "DiagnosticsTestSuite.hpp"

#include "../../diagnostics/Diagnostic.hpp"
#include "../../diagnostics/DiagnosticBuilder.hpp"
#include "../../diagnostics/DiagnosticRenderer.hpp"
#include "../../diagnostics/ErrorCodeRegistry.hpp"
#include "../../diagnostics/ExceptionConverter.hpp"
#include "../../diagnostics/SourceFileCache.hpp"

#include "../../analysis/OverrideAnnotationChecker.hpp"
#include "../../analysis/UnusedVariableAnalyzer.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../value/ValueType.hpp"

#include "../../errors/AccessViolationException.hpp"
#include "../../errors/AmbiguousCallException.hpp"
#include "../../errors/ClassNotFoundException.hpp"
#include "../../errors/DuplicateDeclarationException.hpp"
#include "../../errors/FieldNotFoundException.hpp"
#include "../../errors/MethodNotFoundException.hpp"
#include "../../errors/MissingSemicolonException.hpp"
#include "../../errors/NoMatchingOverloadException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/TypeConversionException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/TypeResolutionException.hpp"
#include "../../errors/UndefinedException.hpp"

#include "../../util/DidYouMean.hpp"
#include "../../util/EditDistance.hpp"

#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;

    namespace
    {
        // Throw on failure — the NATIVE_CALLBACK runner converts this into
        // a FAILED test with the message attached.
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }
    }

    void DiagnosticsTestSuite::setupTests()
    {
        // ================================================================
        // util::levenshtein
        // ================================================================

        addCallbackTest("levenshtein: identical strings have distance 0", "",
            [](ScriptAPI&) {
                require(util::levenshtein("foo", "foo", 5) == 0,
                    "expected 0 for identical strings");
            });

        addCallbackTest("levenshtein: empty vs N has distance N", "",
            [](ScriptAPI&) {
                require(util::levenshtein("", "abc", 5) == 3, "empty -> 'abc' should be 3");
                require(util::levenshtein("abc", "", 5) == 3, "'abc' -> empty should be 3");
                require(util::levenshtein("", "", 5) == 0, "empty -> empty should be 0");
            });

        addCallbackTest("levenshtein: single substitution", "",
            [](ScriptAPI&) {
                require(util::levenshtein("foo", "fou", 5) == 1,
                    "foo -> fou should be 1 substitution");
            });

        addCallbackTest("levenshtein: classic kitten/sitting = 3", "",
            [](ScriptAPI&) {
                require(util::levenshtein("kitten", "sitting", 5) == 3,
                    "kitten -> sitting is the textbook distance 3");
            });

        addCallbackTest("levenshtein: respects maxDistance early-out", "",
            [](ScriptAPI&) {
                // 7 edits required, but we only ask for budget 2 — must
                // return budget+1 == 3 (not the true distance 7).
                const int result = util::levenshtein("abcdefg", "1234567", 2);
                require(result == 3,
                    "expected early-out to return maxDistance+1 == 3, got "
                    + std::to_string(result));
            });

        addCallbackTest("levenshtein: length-difference quick reject", "",
            [](ScriptAPI&) {
                // |m - n| = 5 > maxDistance 2, must short-circuit.
                const int result = util::levenshtein("a", "abcdef", 2);
                require(result == 3,
                    "expected length-difference reject (3), got "
                    + std::to_string(result));
            });

        // ================================================================
        // util::findClosestMatch
        // ================================================================

        addCallbackTest("didYouMean: returns nullopt for empty pool", "",
            [](ScriptAPI&) {
                std::vector<std::string> empty;
                auto result = util::findClosestMatch("foo", empty);
                require(!result.has_value(), "expected nullopt for empty pool");
            });

        addCallbackTest("didYouMean: finds typo within budget", "",
            [](ScriptAPI&) {
                const std::vector<std::string> pool = { "foo", "bar", "baz" };
                auto result = util::findClosestMatch("foa", pool);
                require(result.has_value(), "expected a match for 'foa'");
                require(*result == "foo",
                    "expected 'foo', got '" + *result + "'");
            });

        addCallbackTest("didYouMean: skips exact matches", "",
            [](ScriptAPI&) {
                // 'foo' is in the pool and matches exactly — must skip
                // and return the next-best (none, since 'bar' is too far).
                const std::vector<std::string> pool = { "foo", "bar" };
                auto result = util::findClosestMatch("foo", pool);
                require(!result.has_value(),
                    "expected nullopt when query exactly matches a candidate");
            });

        addCallbackTest("didYouMean: respects rustc heuristic budget", "",
            [](ScriptAPI&) {
                // Query length 3 → budget = max(1, 3/3) = 1.
                // 'aaa' vs 'bbb' is distance 3, must reject.
                const std::vector<std::string> pool = { "bbb" };
                auto result = util::findClosestMatch("aaa", pool);
                require(!result.has_value(),
                    "expected nullopt when distance exceeds rustc budget");
            });

        addCallbackTest("didYouMean: tie-break is alphabetic", "",
            [](ScriptAPI&) {
                // Both 'foa' and 'foc' are distance 1 from 'fob' — should
                // pick 'foa' deterministically.
                const std::vector<std::string> pool = { "foc", "foa" };
                auto result = util::findClosestMatch("fob", pool);
                require(result.has_value(), "expected a match");
                require(*result == "foa",
                    "expected alphabetic tie-break 'foa', got '" + *result + "'");
            });

        addCallbackTest("didYouMean: handles longer identifiers", "",
            [](ScriptAPI&) {
                // length 12 → budget = max(1, 4) = 4 (capped at default 3).
                // Effective budget = min(3, 4) = 3 — distance 1 fits.
                const std::vector<std::string> pool = { "calculateSum", "totalize" };
                auto result = util::findClosestMatch("calculateSun", pool);
                require(result.has_value(), "expected a match for one-letter typo");
                require(*result == "calculateSum",
                    "expected 'calculateSum', got '" + *result + "'");
            });

        // ================================================================
        // diagnostics::SourceFileCache
        // ================================================================

        addCallbackTest("SourceFileCache: publish + getLine round-trip", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("test/cache_round_trip.mt");
                cache.publishFromContent(
                    "test/cache_round_trip.mt",
                    "first line\nsecond line\nthird line");

                auto line1 = cache.getLine("test/cache_round_trip.mt", 1);
                auto line2 = cache.getLine("test/cache_round_trip.mt", 2);
                auto line3 = cache.getLine("test/cache_round_trip.mt", 3);
                auto line4 = cache.getLine("test/cache_round_trip.mt", 4);

                require(line1.has_value() && *line1 == "first line",
                    "line 1 missing or wrong");
                require(line2.has_value() && *line2 == "second line",
                    "line 2 missing or wrong");
                require(line3.has_value() && *line3 == "third line",
                    "line 3 missing or wrong");
                require(!line4.has_value(),
                    "out-of-range line should be nullopt");

                cache.invalidate("test/cache_round_trip.mt");
            });

        addCallbackTest("SourceFileCache: handles CRLF line endings", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("test/cache_crlf.mt");
                cache.publishFromContent(
                    "test/cache_crlf.mt",
                    "alpha\r\nbeta\r\ngamma");

                auto line1 = cache.getLine("test/cache_crlf.mt", 1);
                auto line2 = cache.getLine("test/cache_crlf.mt", 2);

                require(line1.has_value() && *line1 == "alpha",
                    "CRLF line 1 should strip the carriage return");
                require(line2.has_value() && *line2 == "beta",
                    "CRLF line 2 should strip the carriage return");

                cache.invalidate("test/cache_crlf.mt");
            });

        addCallbackTest("SourceFileCache: invalidate drops the file", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("test/cache_invalidate.mt", "only line");
                require(cache.getLineCount("test/cache_invalidate.mt") == 1,
                    "expected 1 line before invalidate");
                cache.invalidate("test/cache_invalidate.mt");
                require(cache.getLineCount("test/cache_invalidate.mt") == 0,
                    "expected 0 lines after invalidate");
            });

        addCallbackTest("SourceFileCache: filename normalization", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.invalidate("foo/bar/baz.mt");
                cache.publishFromContent("foo/bar/baz.mt", "content");

                // Backslashes and forward slashes should resolve to the
                // same cache entry. Double separators should also collapse.
                auto a = cache.getLine("foo\\bar\\baz.mt", 1);
                auto b = cache.getLine("foo//bar/baz.mt", 1);
                auto c = cache.getLine("foo/bar/baz.mt", 1);

                require(a.has_value() && *a == "content",
                    "backslash variant should hit the same entry");
                require(b.has_value() && *b == "content",
                    "double-slash variant should hit the same entry");
                require(c.has_value() && *c == "content",
                    "canonical form should hit");

                cache.invalidate("foo/bar/baz.mt");
            });

        addCallbackTest("SourceFileCache: getLine rejects line < 1", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("test/cache_zero.mt", "x");
                auto result = cache.getLine("test/cache_zero.mt", 0);
                require(!result.has_value(), "line 0 should be rejected");
                cache.invalidate("test/cache_zero.mt");
            });

        addCallbackTest("SourceFileCache: normalizeFilename collapses separators", "",
            [](ScriptAPI&) {
                using diagnostics::SourceFileCache;
                require(SourceFileCache::normalizeFilename("a\\b\\c") == "a/b/c",
                    "backslashes should become forward slashes");
                require(SourceFileCache::normalizeFilename("a//b///c") == "a/b/c",
                    "consecutive slashes should collapse");
                require(SourceFileCache::normalizeFilename("a/b/c") == "a/b/c",
                    "already-normalized form should pass through");
            });

        // ================================================================
        // diagnostics::fromException — exception → Diagnostic conversion
        // ================================================================

        addCallbackTest("converter: ParseException maps to ParseInvalidSyntax", "",
            [](ScriptAPI&) {
                errors::ParseException ex("unexpected token",
                    errors::SourceLocation("file.mt", 5, 10));
                auto diag = diagnostics::fromException(ex);
                require(diag.code != nullptr, "diagnostic should have a code");
                require(std::string(diag.code->id) == "MT-E0005",
                    "expected MT-E0005, got " + std::string(diag.code->id));
                require(diag.severity == diagnostics::Severity::Error,
                    "ParseException should be Error severity");
                require(diag.primarySpan.start.getLine() == 5,
                    "primary span line should be 5");
                require(diag.primarySpan.start.getColumn() == 10,
                    "primary span column should be 10");
                require(diag.sourceExceptionType == "ParseException",
                    "exception type tag should be ParseException");
            });

        addCallbackTest("converter: TypeException maps to TypeGeneric", "",
            [](ScriptAPI&) {
                errors::TypeException ex("type mismatch",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E2007",
                    "expected MT-E2007 generic type code");
            });

        addCallbackTest("converter: RuntimeException strips runtime prefix", "",
            [](ScriptAPI&) {
                errors::RuntimeException ex("division by zero",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E5005",
                    "expected MT-E5005 runtime catchall");
                // The constructor adds "Runtime error: " — converter should strip it.
                require(diag.message == "division by zero",
                    "expected 'division by zero', got '" + diag.message + "'");
            });

        addCallbackTest("converter: ClassNotFoundException carries class name", "",
            [](ScriptAPI&) {
                errors::ClassNotFoundException ex("Foo",
                    errors::SourceLocation("file.mt", 3, 5));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E1006",
                    "expected MT-E1006 NameClassNotFound");
                require(diag.message.find("Foo") != std::string::npos,
                    "message should contain class name");
                require(diag.primarySpan.label == "not found in this scope",
                    "primary span label should mention scope");
            });

        addCallbackTest("converter: MethodNotFoundException with class name", "",
            [](ScriptAPI&) {
                errors::MethodNotFoundException ex("doStuff", "Foo",
                    errors::SourceLocation("file.mt", 7, 3));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E1007",
                    "expected MT-E1007 NameMethodNotFound");
                require(diag.message.find("doStuff") != std::string::npos,
                    "message should contain method name");
                require(diag.message.find("Foo") != std::string::npos,
                    "message should contain class name");
            });

        addCallbackTest("converter: FieldNotFoundException without class name", "",
            [](ScriptAPI&) {
                errors::FieldNotFoundException ex("count",
                    errors::SourceLocation("file.mt", 2, 4));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E1008",
                    "expected MT-E1008 NameFieldNotFound");
            });

        addCallbackTest("converter: NullPointerException carries operation note", "",
            [](ScriptAPI&) {
                errors::NullPointerException ex("oops", "field access",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E5001",
                    "expected MT-E5001 RuntimeNullPointer");
                bool foundOperation = false;
                for (const auto& note : diag.notes)
                {
                    if (note.find("field access") != std::string::npos)
                    {
                        foundOperation = true;
                        break;
                    }
                }
                require(foundOperation,
                    "notes should include the operation context");
            });

        addCallbackTest("converter: TypeConversionException records source/target", "",
            [](ScriptAPI&) {
                errors::TypeConversionException ex("incompatible", "Int", "String",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E2002",
                    "expected MT-E2002 TypeConversionFailed");
                require(diag.message.find("Int") != std::string::npos
                        && diag.message.find("String") != std::string::npos,
                    "message should reference both source and target types");
            });

        addCallbackTest("converter: TypeResolutionException maps to MT-E2003", "",
            [](ScriptAPI&) {
                errors::TypeResolutionException ex("Unknown type: Foo",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E2003",
                    "expected MT-E2003 TypeResolutionFailed");
            });

        addCallbackTest("converter: AmbiguousCallException emits candidate notes", "",
            [](ScriptAPI&) {
                std::vector<std::string> candidates = { "Int, Int", "Float, Float" };
                std::vector<std::string> args = { "Int", "Float" };
                errors::AmbiguousCallException ex("add", candidates, args,
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E2004",
                    "expected MT-E2004 TypeAmbiguousCall");
                // Two candidates + 1 hint = 3 notes
                require(diag.notes.size() == 3,
                    "expected 3 notes (2 candidates + 1 hint), got "
                    + std::to_string(diag.notes.size()));
            });

        addCallbackTest("converter: NoMatchingOverloadException with no candidates", "",
            [](ScriptAPI&) {
                errors::NoMatchingOverloadException ex("foo", {}, { "Int" },
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E2005",
                    "expected MT-E2005 TypeNoMatchingOverload");
                require(diag.notes.size() == 1, "expected 1 'no overloads' note");
            });

        addCallbackTest("converter: AccessViolationException emits notes for PRIVATE", "",
            [](ScriptAPI&) {
                errors::AccessViolationException ex(
                    "secret", "field", ast::AccessModifier::PRIVATE,
                    "Vault", "main",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E3001",
                    "expected MT-E3001 AccessViolation");
                require(!diag.notes.empty(),
                    "AccessViolationException should produce notes");
            });

        addCallbackTest("converter: DuplicateDeclarationException uses two spans", "",
            [](ScriptAPI&) {
                errors::DuplicateDeclarationException ex(
                    "class", "Foo",
                    errors::SourceLocation("a.mt", 5, 1),
                    errors::SourceLocation("a.mt", 12, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E0003",
                    "expected MT-E0003 ParseDuplicateDeclaration");
                require(diag.primarySpan.start.getLine() == 12,
                    "primary span should be the second declaration");
                require(diag.secondarySpans.size() == 1,
                    "should have one secondary span pointing at the first declaration");
                require(diag.secondarySpans[0].start.getLine() == 5,
                    "secondary span should be at line 5");
                require(diag.secondarySpans[0].label.find("first declared") != std::string::npos,
                    "secondary span label should say 'first declared'");
            });

        addCallbackTest("converter: UndefinedException maps to NameUndefinedVariable", "",
            [](ScriptAPI&) {
                errors::UndefinedException ex("cannot find 'foo'",
                    errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(ex);
                require(std::string(diag.code->id) == "MT-E1001",
                    "expected MT-E1001 NameUndefinedVariable");
            });

        addCallbackTest("converter: std::exception fallback is InternalError", "",
            [](ScriptAPI&) {
                std::runtime_error e("not a script exception");
                auto diag = diagnostics::fromException(e);
                require(std::string(diag.code->id) == "MT-E9000",
                    "expected MT-E9000 InternalError fallback");
                require(diag.sourceExceptionType == "std::exception",
                    "fallback should tag as std::exception");
                require(!diag.hasLocation(),
                    "fallback should have no source location");
            });

        // ================================================================
        // DiagnosticRenderer — Rust-style output (golden, ANSI-stripped)
        // ================================================================

        addCallbackTest("renderer: header includes severity, code, and message", "",
            [](ScriptAPI&) {
                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::NameUndefinedVariable)
                    .withMessage("cannot find value 'foa' in this scope")
                    .withPrimary(errors::SourceLocation("a.mt", 1, 1))
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer renderer(opts);
                renderer.render(diag);

                const std::string output = out.str();
                require(output.find("error[MT-E1001]") != std::string::npos,
                    "header should contain 'error[MT-E1001]'");
                require(output.find("cannot find value 'foa'") != std::string::npos,
                    "header should contain the message");
            });

        addCallbackTest("renderer: snippet block draws caret under the span", "",
            [](ScriptAPI&) {
                // Publish a fake source line so the renderer can fetch it.
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("renderer_caret.mt",
                    "let x = foo();\nlet y = foa();\nlet z = bar();");

                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::NameUndefinedVariable)
                    .withMessage("cannot find 'foa'")
                    .withPrimary(errors::SourceLocation("renderer_caret.mt", 2, 9), "not found")
                    .build();
                diag.primarySpan.end =
                    errors::SourceLocation("renderer_caret.mt", 2, 12);

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer renderer(opts);
                renderer.render(diag);
                const std::string output = out.str();

                require(output.find("--> renderer_caret.mt:2:9") != std::string::npos,
                    "should contain the arrow line with file:line:col");
                require(output.find("let y = foa();") != std::string::npos,
                    "should print the source line");
                require(output.find("^^^") != std::string::npos,
                    "caret should span 3 columns (col 9..12)");
                require(output.find("not found") != std::string::npos,
                    "label should appear after the caret");

                cache.invalidate("renderer_caret.mt");
            });

        addCallbackTest("renderer: notes appear under '= note:' lines", "",
            [](ScriptAPI&) {
                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::TypeMismatch)
                    .withMessage("type mismatch")
                    .withPrimary(errors::SourceLocation("a.mt", 1, 1))
                    .withNote("expected Int, found String")
                    .withNote("the function returns String")
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("= note: expected Int, found String") != std::string::npos,
                    "first note should be rendered");
                require(output.find("= note: the function returns String") != std::string::npos,
                    "second note should be rendered");
            });

        addCallbackTest("renderer: secondary span is printed with its own arrow", "",
            [](ScriptAPI&) {
                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("renderer_dup.mt",
                    "class Foo {}\nclass Bar {}\nclass Foo {}");

                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::ParseDuplicateDeclaration)
                    .withMessage("duplicate class declaration: 'Foo'")
                    .withPrimary(errors::SourceLocation("renderer_dup.mt", 3, 7), "redeclared here")
                    .withSecondary(errors::SourceLocation("renderer_dup.mt", 1, 7), "first declared here")
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("--> renderer_dup.mt:3:7") != std::string::npos,
                    "primary arrow should appear");
                require(output.find("--> renderer_dup.mt:1:7") != std::string::npos,
                    "secondary arrow should appear");
                require(output.find("first declared here") != std::string::npos,
                    "secondary label should appear");
                require(output.find("redeclared here") != std::string::npos,
                    "primary label should appear");

                cache.invalidate("renderer_dup.mt");
            });

        addCallbackTest("renderer: ColorMode::Never produces no escape codes", "",
            [](ScriptAPI&) {
                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::TypeMismatch)
                    .withMessage("type mismatch")
                    .withPrimary(errors::SourceLocation("a.mt", 1, 1))
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("\033[") == std::string::npos,
                    "Never mode must not emit any ANSI escape sequences");
            });

        addCallbackTest("renderer: ColorMode::Always emits escape codes", "",
            [](ScriptAPI&) {
                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::TypeMismatch)
                    .withMessage("type mismatch")
                    .withPrimary(errors::SourceLocation("a.mt", 1, 1))
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Always;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("\033[") != std::string::npos,
                    "Always mode should emit at least one ANSI escape");
            });

        addCallbackTest("renderer: missing source line falls back gracefully", "",
            [](ScriptAPI&) {
                // Don't publish anything for this filename — the renderer
                // should print "<source unavailable>" rather than crashing.
                auto diag = diagnostics::DiagnosticBuilder(diagnostics::codes::TypeGeneric)
                    .withMessage("type error")
                    .withPrimary(errors::SourceLocation("never_published.mt", 42, 7))
                    .build();

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("<source unavailable>") != std::string::npos,
                    "should fall back to <source unavailable>");
                require(output.find("--> never_published.mt:42:7") != std::string::npos,
                    "arrow line should still appear");
            });

        addCallbackTest("renderer: diagnostic without location skips snippet block", "",
            [](ScriptAPI&) {
                std::runtime_error e("internal compiler crash");
                auto diag = diagnostics::fromException(e);

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("error[MT-E9000]") != std::string::npos,
                    "header should still appear for locationless diagnostics");
                require(output.find("internal compiler crash") != std::string::npos,
                    "message should appear");
                require(output.find("-->") == std::string::npos,
                    "no arrow line should appear when there is no location");
            });

        // ================================================================
        // Phase 4 — suggestion-aware converters
        // ================================================================

        addCallbackTest("phase4: UndefinedException with pool emits did-you-mean", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "foo", "bar", "baz" };
                errors::UndefinedException ex(
                    "Variable 'foa' is not defined",
                    errors::SourceLocation("a.mt", 3, 5),
                    pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected 1 suggestion attached to diagnostic, got "
                    + std::to_string(diag.suggestions.size()));
                require(diag.suggestions[0].label.find("foo") != std::string::npos,
                    "suggestion label should mention 'foo'");
            });

        addCallbackTest("phase4: UndefinedException without pool emits no suggestion", "",
            [](ScriptAPI&) {
                errors::UndefinedException ex(
                    "Variable 'foa' is not defined",
                    errors::SourceLocation("a.mt", 3, 5));
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.empty(),
                    "no pool means no suggestions");
            });

        addCallbackTest("phase4: ClassNotFoundException with pool emits did-you-mean", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "List", "Stream", "Queue" };
                errors::ClassNotFoundException ex("Lis",
                    errors::SourceLocation("a.mt", 1, 1), pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected one class suggestion");
                require(diag.suggestions[0].label.find("List") != std::string::npos,
                    "suggestion should propose 'List'");
            });

        addCallbackTest("phase4: MethodNotFoundException with pool emits did-you-mean", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "doStuff", "doThing", "doOther" };
                errors::MethodNotFoundException ex("doStufg", "Foo",
                    errors::SourceLocation("a.mt", 4, 9), pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected one method suggestion");
                require(diag.suggestions[0].label.find("doStuff") != std::string::npos,
                    "suggestion should propose 'doStuff'");
            });

        addCallbackTest("phase4: FieldNotFoundException with pool emits did-you-mean", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "count", "name", "value" };
                errors::FieldNotFoundException ex("counr", "Foo",
                    errors::SourceLocation("a.mt", 6, 5), pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected one field suggestion");
                require(diag.suggestions[0].label.find("count") != std::string::npos,
                    "suggestion should propose 'count'");
            });

        addCallbackTest("phase4: pool too sparse to suggest stays empty", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "totallyDifferent" };
                errors::ClassNotFoundException ex("Foo",
                    errors::SourceLocation("a.mt", 1, 1), pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.empty(),
                    "no candidate within Levenshtein budget should yield no suggestion");
            });

        // ================================================================
        // Phase 5 — quick-fix contract: Suggestion::label format
        //
        // The LSP CodeActionHandler::generateTypoFixActions parses the
        // candidate identifier out of the Suggestion's label by matching
        // the substring between the first pair of single quotes. These
        // tests pin the format that the converter emits so the parser
        // contract can never silently drift.
        // ================================================================

        addCallbackTest("phase5: typo suggestion label has parseable single-quoted candidate", "",
            [](ScriptAPI&) {
                // Pool chosen so exactly one candidate is the closest:
                // 'foo' is distance 1 from 'foa' (substitute a->o), the
                // others are far enough to fall outside the rustc-style
                // budget. Avoids the three-way distance-1 tie that
                // makes 'print/printf/println' style pools brittle.
                std::vector<std::string> pool = { "foo", "totallyDifferent", "anotherIdent" };
                errors::UndefinedException ex(
                    "Variable 'foa' is not defined",
                    errors::SourceLocation("a.mt", 1, 1),
                    pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected exactly one suggestion");

                const std::string& label = diag.suggestions[0].label;
                const size_t openQuote = label.find('\'');
                require(openQuote != std::string::npos,
                    "label must contain an opening single quote");
                const size_t closeQuote = label.find('\'', openQuote + 1);
                require(closeQuote != std::string::npos,
                    "label must contain a closing single quote");
                const std::string candidate =
                    label.substr(openQuote + 1, closeQuote - openQuote - 1);
                require(!candidate.empty(),
                    "candidate identifier must be non-empty");
                require(candidate.find('\'') == std::string::npos,
                    "candidate must not itself contain quotes");
                require(candidate == "foo",
                    "expected closest-match candidate 'foo', got '" + candidate + "'");
            });

        addCallbackTest("phase5: typo suggestion has rendered hint for CLI renderer", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "List" };
                errors::ClassNotFoundException ex("Lis",
                    errors::SourceLocation("a.mt", 1, 1), pool);
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1, "expected one suggestion");
                require(!diag.suggestions[0].renderedHint.empty(),
                    "rendered hint must be set so the CLI renderer can show '= help: ...'");
                require(diag.suggestions[0].renderedHint.find("List") != std::string::npos,
                    "rendered hint should mention the candidate");
            });

        // ================================================================
        // MYT-48 — MissingSemicolonException → MT-E0002 + Insert ';' fix
        // ================================================================

        addCallbackTest("phase48: missing_semicolon_routes_to_MT_E0002", "",
            [](ScriptAPI&) {
                errors::MissingSemicolonException ex(
                    errors::SourceLocation("a.mt", 3, 14));
                auto diag = diagnostics::fromException(ex);
                require(diag.code != nullptr, "expected a code");
                require(std::string(diag.code->id) == "MT-E0002",
                    "expected MT-E0002, got " + std::string(diag.code->id));
                require(diag.severity == diagnostics::Severity::Error,
                    "missing-semicolon should be Error severity");
                require(diag.sourceExceptionType == "MissingSemicolonException",
                    "exception type tag should be MissingSemicolonException");
                require(diag.primarySpan.start.getLine() == 3
                        && diag.primarySpan.start.getColumn() == 14,
                    "primary span should be 3:14");
            });

        addCallbackTest("phase48: missing_semicolon_diagnostic_has_machine_applicable_fix", "",
            [](ScriptAPI&) {
                errors::MissingSemicolonException ex(
                    errors::SourceLocation("a.mt", 1, 7));
                auto diag = diagnostics::fromException(ex);
                require(diag.suggestions.size() == 1,
                    "expected exactly one suggestion, got "
                    + std::to_string(diag.suggestions.size()));

                const auto& fix = diag.suggestions[0];
                require(fix.applicability == diagnostics::FixApplicability::MachineApplicable,
                    "expected MachineApplicable fix");
                require(fix.edits.size() == 1, "expected one TextEdit");
                require(fix.edits[0].newText == ";",
                    "edit must insert exactly ';'");
                // Zero-width insertion (start == end at the same point).
                require(fix.edits[0].start.getLine() == 1
                        && fix.edits[0].start.getColumn() == 7,
                    "edit start should match primary span");
                require(fix.edits[0].end.getLine() == 1
                        && fix.edits[0].end.getColumn() == 7,
                    "edit end should equal start (zero-width insert)");
            });

        addCallbackTest("phase48: missing_semicolon_dispatch_beats_generic_parse_exception", "",
            [](ScriptAPI&) {
                // dynamic_cast must hit the typed branch before the generic
                // ParseException branch — otherwise the code would be MT-E0005.
                std::unique_ptr<errors::ParseException> ex =
                    std::make_unique<errors::MissingSemicolonException>(
                        errors::SourceLocation("a.mt", 1, 1));
                auto diag = diagnostics::fromException(*ex);
                require(std::string(diag.code->id) == "MT-E0002",
                    "expected typed dispatch to MT-E0002, got "
                    + std::string(diag.code->id));
            });

        // ================================================================
        // MYT-50 — OverrideAnnotationChecker → MT-W2002 + Add @Override
        // ================================================================

        // Helper for Phase 50 tests: build a parent + child class pair,
        // optionally annotate the child method with @Override, return the
        // pair so the test can pass it to OverrideAnnotationChecker::check.
        auto buildOverridePair = [](bool childHasOverrideAnnotation,
                                    ast::AccessModifier parentAccess
                                        = ast::AccessModifier::PUBLIC)
            -> std::pair<std::shared_ptr<runtimeTypes::klass::ClassDefinition>,
                         std::shared_ptr<runtimeTypes::klass::ClassDefinition>>
        {
            using runtimeTypes::klass::ClassDefinition;
            using runtimeTypes::klass::MethodDefinition;

            auto parent = std::make_shared<ClassDefinition>("Parent");
            auto child  = std::make_shared<ClassDefinition>("Child");
            child->setParentClass(parent);

            std::vector<std::pair<std::string, value::ParameterType>> noParams;

            auto parentMethod = std::make_shared<MethodDefinition>(
                "foo", value::ValueType::VOID, noParams,
                nullptr, false, parentAccess);
            parentMethod->setSourceLocation(
                errors::SourceLocation("parent.mt", 5, 5));
            parent->addInstanceMethod("foo", parentMethod);

            auto childMethod = std::make_shared<MethodDefinition>(
                "foo", value::ValueType::VOID, noParams,
                nullptr, false, ast::AccessModifier::PUBLIC);
            childMethod->setSourceLocation(
                errors::SourceLocation("child.mt", 12, 5));
            if (childHasOverrideAnnotation) {
                childMethod->addAnnotation(
                    std::make_shared<ast::nodes::annotations::AnnotationNode>("Override"));
            }
            child->addInstanceMethod("foo", childMethod);

            return { parent, child };
        };

        addCallbackTest("phase50: method_overriding_without_annotation_warns",
            "", [buildOverridePair](ScriptAPI&) {
                auto pair = buildOverridePair(false, ast::AccessModifier::PUBLIC);
                auto warns = analysis::OverrideAnnotationChecker::check(*pair.second);
                require(warns.size() == 1,
                    "expected exactly one missing-@Override warning, got "
                    + std::to_string(warns.size()));
                require(std::string(warns[0].code->id) == "MT-W2002",
                    "expected MT-W2002, got " + std::string(warns[0].code->id));
                require(warns[0].severity == diagnostics::Severity::Warning,
                    "missing-@Override should be Warning severity");
            });

        addCallbackTest("phase50: method_with_override_annotation_not_warned",
            "", [buildOverridePair](ScriptAPI&) {
                auto pair = buildOverridePair(true, ast::AccessModifier::PUBLIC);
                auto warns = analysis::OverrideAnnotationChecker::check(*pair.second);
                require(warns.empty(),
                    "method with @Override should not produce a warning");
            });

        addCallbackTest("phase50: method_not_overriding_parent_not_warned",
            "", [](ScriptAPI&) {
                using runtimeTypes::klass::ClassDefinition;
                using runtimeTypes::klass::MethodDefinition;

                auto parent = std::make_shared<ClassDefinition>("Parent");
                auto child  = std::make_shared<ClassDefinition>("Child");
                child->setParentClass(parent);

                std::vector<std::pair<std::string, value::ParameterType>> noParams;
                auto childMethod = std::make_shared<MethodDefinition>(
                    "uniqueToChild", value::ValueType::VOID, noParams,
                    nullptr, false, ast::AccessModifier::PUBLIC);
                childMethod->setSourceLocation(errors::SourceLocation("c.mt", 1, 1));
                child->addInstanceMethod("uniqueToChild", childMethod);

                auto warns = analysis::OverrideAnnotationChecker::check(*child);
                require(warns.empty(),
                    "method unique to child must not be flagged as override");
            });

        addCallbackTest("phase50: private_parent_method_not_warned",
            "", [buildOverridePair](ScriptAPI&) {
                auto pair = buildOverridePair(false, ast::AccessModifier::PRIVATE);
                auto warns = analysis::OverrideAnnotationChecker::check(*pair.second);
                require(warns.empty(),
                    "private parent methods are not visible — child is not overriding");
            });

        addCallbackTest("phase50: warning_has_secondary_span_at_parent",
            "", [buildOverridePair](ScriptAPI&) {
                auto pair = buildOverridePair(false, ast::AccessModifier::PUBLIC);
                auto warns = analysis::OverrideAnnotationChecker::check(*pair.second);
                require(warns.size() == 1, "expected one warning");
                require(warns[0].secondarySpans.size() == 1,
                    "expected one secondary span pointing at parent");
                require(warns[0].secondarySpans[0].start.getFilename() == "parent.mt"
                        && warns[0].secondarySpans[0].start.getLine() == 5,
                    "secondary span should target the parent method's source location");
                require(warns[0].secondarySpans[0].label.find("parent") != std::string::npos,
                    "secondary span label should mention 'parent'");
            });

        addCallbackTest("phase50: warning_suggestion_inserts_override",
            "", [buildOverridePair](ScriptAPI&) {
                auto pair = buildOverridePair(false, ast::AccessModifier::PUBLIC);
                auto warns = analysis::OverrideAnnotationChecker::check(*pair.second);
                require(warns.size() == 1, "expected one warning");
                require(warns[0].suggestions.size() == 1,
                    "expected one Add @Override suggestion");
                const auto& fix = warns[0].suggestions[0];
                require(fix.applicability == diagnostics::FixApplicability::MachineApplicable,
                    "Add @Override fix should be MachineApplicable");
                require(fix.edits.size() == 1, "expected one TextEdit");
                require(fix.edits[0].newText.find("@Override") != std::string::npos,
                    "edit must insert @Override");
                require(fix.edits[0].newText.find("\n") != std::string::npos,
                    "edit must include a newline so the annotation sits on its own line");
            });

        addCallbackTest("phase50: orphan_class_with_no_parent_skipped",
            "", [](ScriptAPI&) {
                using runtimeTypes::klass::ClassDefinition;
                auto orphan = std::make_shared<ClassDefinition>("Orphan");
                auto warns = analysis::OverrideAnnotationChecker::check(*orphan);
                require(warns.empty(),
                    "classes without a parent should produce no warnings");
            });

        addCallbackTest("phase5: rendered diagnostic includes help line", "",
            [](ScriptAPI&) {
                std::vector<std::string> pool = { "calculateSum" };
                errors::UndefinedException ex(
                    "Variable 'calculateSun' is not defined",
                    errors::SourceLocation("phase5_help.mt", 2, 1),
                    pool);

                auto& cache = diagnostics::SourceFileCache::instance();
                cache.publishFromContent("phase5_help.mt",
                    "let calculateSum = 1;\nprintln(calculateSun);");

                auto diag = diagnostics::fromException(ex);

                std::ostringstream out;
                auto opts = diagnostics::DiagnosticRenderer::defaultOptions();
                opts.stream = &out;
                opts.colorMode = diagnostics::DiagnosticRenderer::ColorMode::Never;
                diagnostics::DiagnosticRenderer(opts).render(diag);
                const std::string output = out.str();

                require(output.find("= help:") != std::string::npos,
                    "renderer must emit '= help:' line for diagnostics with suggestions");
                require(output.find("calculateSum") != std::string::npos,
                    "help line must include the candidate identifier");

                cache.invalidate("phase5_help.mt");
            });

        // ================================================================
        // MYT-49 — UnusedVariableAnalyzer → MT-W2001 + rename/remove fixes
        // ================================================================

        addCallbackTest("phase49: unused_top_level_variable_warns",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                auto value = std::make_unique<expressions::IntegerNode>(
                    42, errors::SourceLocation("a.mt", 1, 9));
                auto decl = std::make_unique<statements::AssignmentNode>(
                    "foo", std::move(value),
                    value::ValueType::INT, "", false, false,
                    errors::SourceLocation("a.mt", 1, 5));

                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                stmts.push_back(std::move(decl));
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));

                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.size() == 1,
                    "expected one unused-variable warning, got "
                    + std::to_string(warns.size()));
                require(std::string(warns[0].code->id) == "MT-W2001",
                    "expected MT-W2001, got " + std::string(warns[0].code->id));
                require(warns[0].severity == diagnostics::Severity::Warning,
                    "should be Warning severity");
                require(warns[0].message.find("foo") != std::string::npos,
                    "message should mention the variable name");
            });

        addCallbackTest("phase49: variable_read_via_VariableNode_not_warned",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                auto value = std::make_unique<expressions::IntegerNode>(
                    42, errors::SourceLocation("a.mt", 1, 9));
                auto decl = std::make_unique<statements::AssignmentNode>(
                    "foo", std::move(value),
                    value::ValueType::INT, "", false, false,
                    errors::SourceLocation("a.mt", 1, 5));

                // Wrap a VariableNode "foo" in a function call to count
                // as a read in expression position.
                std::vector<std::unique_ptr<ast::ASTNode>> args;
                args.push_back(std::make_unique<expressions::VariableNode>(
                    "foo", errors::SourceLocation("a.mt", 2, 9)));
                auto call = std::make_unique<functions::FunctionCallNode>(
                    "println", std::move(args),
                    errors::SourceLocation("a.mt", 2, 1));

                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                stmts.push_back(std::move(decl));
                stmts.push_back(std::move(call));
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));

                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.empty(),
                    "variable read after declaration should produce no warning");
            });

        addCallbackTest("phase49: underscore_prefix_skipped",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                auto value = std::make_unique<expressions::IntegerNode>(
                    42, errors::SourceLocation("a.mt", 1, 10));
                auto decl = std::make_unique<statements::AssignmentNode>(
                    "_unused", std::move(value),
                    value::ValueType::INT, "", false, false,
                    errors::SourceLocation("a.mt", 1, 5));

                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                stmts.push_back(std::move(decl));
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));

                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.empty(),
                    "_-prefixed variables are deliberately marked unused");
            });

        addCallbackTest("phase49: warning_carries_unnecessary_tag",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                auto value = std::make_unique<expressions::IntegerNode>(
                    1, errors::SourceLocation("a.mt", 1, 9));
                auto decl = std::make_unique<statements::AssignmentNode>(
                    "x", std::move(value),
                    value::ValueType::INT, "", false, false,
                    errors::SourceLocation("a.mt", 1, 5));

                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                stmts.push_back(std::move(decl));
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));

                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.size() == 1, "expected one warning");
                bool hasUnnecessaryTag = false;
                for (int t : warns[0].tags) if (t == 1) hasUnnecessaryTag = true;
                require(hasUnnecessaryTag,
                    "warning must carry LSP DiagnosticTag::Unnecessary (1)");
            });

        addCallbackTest("phase49: warning_has_rename_to_underscore_fix",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                auto value = std::make_unique<expressions::IntegerNode>(
                    1, errors::SourceLocation("a.mt", 1, 9));
                auto decl = std::make_unique<statements::AssignmentNode>(
                    "tmp", std::move(value),
                    value::ValueType::INT, "", false, false,
                    errors::SourceLocation("a.mt", 1, 5));

                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                stmts.push_back(std::move(decl));
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));

                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.size() == 1, "expected one warning");
                require(!warns[0].suggestions.empty(),
                    "expected at least one suggestion");
                const auto& fix = warns[0].suggestions[0];
                require(fix.applicability == diagnostics::FixApplicability::MachineApplicable,
                    "rename fix should be MachineApplicable");
                require(fix.edits.size() == 1, "expected one TextEdit");
                require(fix.edits[0].newText == "_",
                    "rename fix inserts a single underscore prefix");
            });

        addCallbackTest("phase49: empty_program_has_no_warnings",
            "", [](ScriptAPI&) {
                using namespace ast::nodes;
                std::vector<std::unique_ptr<ast::ASTNode>> stmts;
                auto program = std::make_unique<statements::ProgramNode>(
                    std::move(stmts), errors::SourceLocation("a.mt", 1, 1));
                auto warns = analysis::UnusedVariableAnalyzer::analyze(program.get());
                require(warns.empty(),
                    "empty program should produce zero warnings");
            });

        addCallbackTest("phase49: null_input_returns_empty",
            "", [](ScriptAPI&) {
                auto warns = analysis::UnusedVariableAnalyzer::analyze(nullptr);
                require(warns.empty(),
                    "null AST should return empty vector, not crash");
            });
    }
}
