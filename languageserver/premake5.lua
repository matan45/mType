-- mType Language Server Build Configuration
workspace "mType-LanguageServer"
    architecture "x64"
    startproject "mtype-language-server"

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

project "mtype-language-server"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir)

    files
    {
        "main.cpp",
        "src/**.hpp",
        "src/**.cpp",

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
        "../mType/runtimeTypes/klass/ObjectInstance.cpp",

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

    includedirs
    {
        "src",
        "../mType",
        "vendor",  -- For local nlohmann/json (vendor/nlohmann/json.hpp)
        -- Also include vcpkg path if available (won't hurt if it doesn't exist)
        "$(VCPKG_ROOT)/installed/x64-windows/include",
    }

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
