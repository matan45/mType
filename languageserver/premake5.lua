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

        -- Minimal mType core sources for LSP (tokenization only)

        -- Lexer components (required for tokenization)
        "../mType/lexer/Lexer.cpp",
        "../mType/lexer/BracketBalancer.cpp",
        "../mType/lexer/SourceLocationTracker.cpp",
        "../mType/lexer/TokenFactory.cpp",

        -- Services (file reading)
        "../mType/services/FileReader.cpp",

        -- Value system (minimal - only what Lexer needs)
        "../mType/value/StringPool.cpp",
        "../mType/value/InternedString.cpp",
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
