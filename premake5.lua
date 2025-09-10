workspace "Interpreter"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "."  -- Place generated solution files in the root directory
   startproject "mType"
   
   
-- Set a consistent location for build artifacts
builddir = "bin/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"

-- Project 1: mType
project "mType"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"

   -- Specify project files and their directory
   location "mType"
   files { "mType/**.hpp", "mType/**.cpp" ,"mType/tests/testFiles/**.mt", "mType/tests/testFiles/**.expected",}
   
    -- Set the target output directory using the shared variable
   targetdir (builddir)

   defines { "_CRT_SECURE_NO_WARNINGS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"



-- Project: assimp and softal need to build with cmake...