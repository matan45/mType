-- mType Language Server Build Configuration
workspace "mType-LanguageServer"
    architecture "x64"
    startproject "mtype-language-server"
	toolset "v145"

    configurations
    {
        "Debug",
        "Release"
    }

    flags
    {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Shared configuration applied to all LSP projects
function lspCommonConfig()
    language "C++"
    cppdialect "C++17"

    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir)

    includedirs
    {
        "src",
        "../mType",
        "vendor",  -- For local nlohmann/json (vendor/nlohmann/json.hpp)
        -- Also include vcpkg path if available (won't hurt if it doesn't exist)
        "$(VCPKG_ROOT)/installed/x64-windows/include",
    }

    -- MYT-35 — The LSP path never throws UserException (it doesn't run
    -- bytecode) and doesn't link mtype-core, so the dispatch is compiled
    -- out via this define. ExceptionConverter.cpp guards the dispatch
    -- and the converter function with #ifndef MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION.
    -- _CRT_SECURE_NO_WARNINGS matches the main mType project's setting so
    -- std::getenv (used by TerminalDetect) doesn't trigger C4996.
    defines { "MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION", "_CRT_SECURE_NO_WARNINGS" }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "On"

    filter {}
end


--------------------------------------------------------------------------------
-- Library: mtype-language-server-lib
-- All LSP handler/analysis code as a static library (excludes entry point)
--------------------------------------------------------------------------------
project "mtype-language-server-lib"
    kind "StaticLib"
    lspCommonConfig()

    files
    {
        "src/**.hpp",
        "src/**.cpp",

        -- MYT-35 — Shared diagnostic data model and exception converter.
        -- The LSP needs the exception → Diagnostic conversion (used by
        -- DocumentManager) and the SourceFileCache (so editor buffers
        -- are addressable for snippets the renderer reads). The Rust-style
        -- DiagnosticRenderer itself is included for build symmetry but
        -- is not actually invoked by the LSP path.
        "../mType/diagnostics/**.hpp",
        "../mType/diagnostics/**.cpp",

        -- MYT-49/MYT-50 — Compile-time analyzer passes (override checker,
        -- unused variable analyzer). The LSP runs OverrideAnnotationChecker
        -- against parsed documents (DocumentManager.cpp) for parity with
        -- the bytecode-compile path.
        "../mType/analysis/**.hpp",
        "../mType/analysis/**.cpp",

        -- MYT-35 — String utilities (Levenshtein, DidYouMean) used by
        -- the suggestion engine paths in later phases. Header-only deps
        -- need the .cpp here so the linker resolves them.
        "../mType/util/**.hpp",
        "../mType/util/**.cpp",

        -- MYT-35 — Errors that the converter dispatches against. Most
        -- exception classes are header-only so their typeinfo is emitted
        -- per-TU and deduped by the linker. AccessViolationException has
        -- an out-of-line .cpp where the typeinfo lives, so we ship it
        -- here for the converter's dynamic_cast to resolve. UserException
        -- is intentionally excluded — it pulls in value::Value (mtype-core)
        -- and the LSP path doesn't run bytecode, so it can never throw one.
        -- ExceptionConverter.cpp's dispatch is compiled out via
        -- MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION (see defines below).
        "../mType/errors/AccessViolationException.cpp",

        -- mType core sources for full semantic analysis

        -- Lexer components
        "../mType/lexer/Lexer.cpp",
        "../mType/lexer/BracketBalancer.cpp",
        "../mType/lexer/SourceLocationTracker.cpp",
        "../mType/lexer/TokenFactory.cpp",

        -- Parser components
        "../mType/parser/Parser.cpp",
        "../mType/parser/ClassParser.cpp",
        "../mType/parser/ExpressionParser.cpp",
        "../mType/parser/InterfaceParser.cpp",
        "../mType/parser/LambdaParser.cpp",
        "../mType/parser/StatementParser.cpp",
        "../mType/parser/TypeParser.cpp",
        "../mType/parser/ParseContext.cpp",
        "../mType/parser/ParserValidator.cpp",
        "../mType/parser/TokenStream.cpp",
        "../mType/parser/TypeRegistry.cpp",

        -- Parser core
        "../mType/parser/core/BaseParser.cpp",

        -- Class parsers
        "../mType/parser/class/GenericParameterParser.cpp",
        "../mType/parser/class/ConstructorParser.cpp",
        "../mType/parser/class/FieldParser.cpp",
        "../mType/parser/class/MethodParser.cpp",
        "../mType/parser/class/ClassDeclarationParser.cpp",
        "../mType/parser/class/ObjectCreationParser.cpp",

        -- Interface parsers
        "../mType/parser/interface/InterfaceMethodSignatureParser.cpp",

        -- Expression parsers
        "../mType/parser/expression/ArgumentParser.cpp",
        "../mType/parser/expression/BinaryOperatorParser.cpp",
        "../mType/parser/expression/CastParser.cpp",
        "../mType/parser/expression/UnaryOperatorParser.cpp",
        "../mType/parser/expression/PostfixOperatorParser.cpp",
        "../mType/parser/expression/LiteralParser.cpp",

        -- Statement parsers
        "../mType/parser/statement/ControlFlowParser.cpp",
        "../mType/parser/statement/ExceptionParser.cpp",
        "../mType/parser/statement/ImportParser.cpp",
        "../mType/parser/statement/DeclarationParser.cpp",
        "../mType/parser/statement/LoopParser.cpp",
        "../mType/parser/statement/AssignmentStatementParser.cpp",
        "../mType/parser/statement/FunctionParser.cpp",

        -- Parser utilities
        "../mType/parser/utilities/StatementTypeDetector.cpp",
        "../mType/parser/utilities/VisibilityParser.cpp",
        "../mType/parser/utilities/AccessModifierParser.cpp",
        "../mType/parser/utilities/NameValidator.cpp",
        "../mType/parser/utilities/AsyncValidator.cpp",
        "../mType/parser/utilities/ParameterParser.cpp",
        "../mType/parser/utilities/QualifiedNameParser.cpp",
        "../mType/parser/utilities/ParserUtils.cpp",
        "../mType/parser/utilities/AnnotationParser.cpp",

        -- AST components
        "../mType/ast/ASTNode.cpp",
        "../mType/ast/AccessModifier.cpp",
        "../mType/ast/GenericType.cpp",
        "../mType/ast/GenericTypeParameter.cpp",
        "../mType/ast/GenericTypeSubstitutionContext.cpp",
        "../mType/ast/nodes/**/*.cpp",
        "../mType/ast/utils/GenericTypeConversionUtils.cpp",

        -- Environment and symbol management
        "../mType/environment/Environment.cpp",
        "../mType/environment/EnvironmentBuilder.cpp",
        "../mType/environment/manager/*.cpp",
        "../mType/environment/registry/ClassRegistry.cpp",
        "../mType/environment/registry/ExportRegistry.cpp",
        "../mType/environment/registry/FunctionRegistry.cpp",
        "../mType/environment/registry/InheritanceTracker.cpp",
        -- Note: Excluding NativeRegistry.cpp to avoid runtime builtin function dependencies

        -- Services
        "../mType/services/FileReader.cpp",
        "../mType/services/ImportManager.cpp",

        -- Type system
        "../mType/types/TypeConversionUtils.cpp",
        "../mType/types/TypeRegistry.cpp",
        "../mType/types/UnifiedType.cpp",
        "../mType/types/ReifiedTypeRegistry.cpp",
        "../mType/types/TypeSubstitutionService.cpp",

        -- VM components (minimal for LSP)
        "../mType/vm/MethodSignature.cpp",

        -- Runtime types (for class/function definitions - minimal set for LSP)
        "../mType/runtimeTypes/Definition.cpp",
        "../mType/runtimeTypes/global/FunctionDefinition.cpp",
        "../mType/runtimeTypes/global/VariableDefinition.cpp",
        -- Note: Excluding ArrayOperationsNative.cpp to avoid SIMD dependencies
        "../mType/runtimeTypes/klass/ClassDefinition.cpp",
        "../mType/runtimeTypes/klass/ConstructorDefinition.cpp",
        "../mType/runtimeTypes/klass/FieldDefinition.cpp",
        "../mType/runtimeTypes/klass/InterfaceDefinition.cpp",
        "../mType/runtimeTypes/klass/InterfaceRegistry.cpp",
        "../mType/runtimeTypes/klass/MethodDefinition.cpp",
        -- Note: ObjectInstance.cpp excluded - using stub in src/stubs/ObjectInstanceStub.cpp

        -- Circular dependency detection
        "../mType/circularDependency/CircularDependencyDetector.cpp",
        "../mType/circularDependency/DependencyPatternAnalyzer.cpp",
        "../mType/circularDependency/DependencyTypeUtils.cpp",

        -- Value system (minimal - only what's needed for parsing)
        "../mType/value/StringPool.cpp",
        "../mType/value/InternedString.cpp",
        "../mType/value/ValueTypeUtils.cpp",
        -- Note: Excluding ArrayOperations.cpp to avoid SIMD/runtime dependencies
    }


--------------------------------------------------------------------------------
-- Executable: mtype-language-server
-- Thin entry point that links the static library
--------------------------------------------------------------------------------
project "mtype-language-server"
    kind "ConsoleApp"
    lspCommonConfig()

    files { "main.cpp" }

    links { "mtype-language-server-lib" }


--------------------------------------------------------------------------------
-- Executable: mtype-language-server-tests
-- Test runner for LSP handler-level tests
--------------------------------------------------------------------------------
project "mtype-language-server-tests"
    kind "ConsoleApp"
    lspCommonConfig()

    includedirs { "tests" }

    files
    {
        "tests/**.hpp",
        "tests/**.cpp",
    }

    links { "mtype-language-server-lib" }
