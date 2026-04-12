-- mType Package Manager Build Configuration
workspace "mType-PackageManager"
    architecture "x64"
    startproject "mtype-package-manager"
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


--------------------------------------------------------------------------------
-- Executable: mtype-package-manager
-- Standalone CLI for package management
--------------------------------------------------------------------------------
project "mtype-package-manager"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir)
    objdir ("obj/" .. outputdir)

    includedirs
    {
        "src",
        "../mType",
    }

    files
    {
        "src/**.hpp",
        "src/**.cpp",
        -- Pull in dependencies from mType that we need
        "../mType/project/ProjectConfigParser.hpp",
        "../mType/project/ProjectConfigParser.cpp",
        "../mType/project/ProjectConfig.hpp",
        "../mType/project/XmlParser.hpp",
        "../mType/project/XmlParser.cpp",
        "../mType/project/GlobMatcher.hpp",
        "../mType/project/GlobMatcher.cpp",
        "../mType/services/FileReader.hpp",
        "../mType/services/FileReader.cpp",
        "../mType/json/JsonParser.hpp",
        "../mType/json/JsonParser.cpp",
        "../mType/json/JsonValue.hpp",
        "../mType/json/JsonValue.cpp",
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
