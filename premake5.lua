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

   defines { "_CRT_SECURE_NO_WARNINGS", "MTYPE_SIMD_ENABLED" }

   -- Platform-specific SIMD configurations
   filter "system:windows"
      systemversion "latest"
      -- SSE2 is already the baseline for x64, no flag needed

      -- AVX2 support for release builds
      filter "configurations:Release"
         buildoptions { "/arch:AVX2" }
      filter {}

   filter "system:linux or system:macosx"
      -- GCC/Clang SIMD flags
      buildoptions { "-msse2", "-msse4.1" }

      -- AVX2 for release builds
      filter "configurations:Release"
         buildoptions { "-mavx2", "-mfma" }
      filter {}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter {}



-- Project: assimp and softal need to build with cmake...