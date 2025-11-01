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

        -- Include mType core sources
        "../mType/lexer/Lexer.cpp",
        "../mType/lexer/BracketBalancer.cpp",
        "../mType/lexer/SourceLocationTracker.cpp",
        "../mType/lexer/TokenFactory.cpp",
        "../mType/parser/Parser.cpp",
        "../mType/parser/ClassParser.cpp",
        "../mType/parser/ExpressionParser.cpp",
        "../mType/parser/InterfaceParser.cpp",
        "../mType/parser/LambdaParser.cpp",
        "../mType/parser/StatementParser.cpp",
        "../mType/parser/TypeParser.cpp",
        "../mType/parser/ParseContext.cpp",
        "../mType/parser/ParserUtils.cpp",
        "../mType/parser/ParserValidator.cpp",
        "../mType/parser/TokenStream.cpp",
        "../mType/environment/Environment.cpp",
        "../mType/environment/EnvironmentBuilder.cpp",
        "../mType/environment/manager/**.cpp",
        "../mType/environment/registry/**.cpp",
    }

    includedirs
    {
        "src",
        "../mType",
        -- Add path to nlohmann/json - adjust as needed
        "$(VCPKG_ROOT)/installed/x64-windows/include",
        -- Or if using a local copy:
        -- "vendor/nlohmann",
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
