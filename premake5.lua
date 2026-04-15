workspace "Interpreter"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "."  -- Place generated solution files in the root directory
   startproject "mType"
   toolset "v145"


-- Set a consistent location for build artifacts
builddir = "bin/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"

-- Shared configuration applied to all projects
function commonConfig()
   language "C++"
   cppdialect "C++20"
   targetdir (builddir)

   defines { "_CRT_SECURE_NO_WARNINGS", "MTYPE_SIMD_ENABLED" }

   -- Platform-specific configurations
   filter "system:windows"
      systemversion "latest"
      buildoptions { "/MP" }

      filter "configurations:Release"
         buildoptions { "/arch:AVX2" }
      filter {}

   filter "system:linux or system:macosx"
      buildoptions { "-msse2", "-msse4.1" }

   filter { "system:linux or system:macosx", "configurations:Release" }
      buildoptions { "-mavx2", "-mfma" }

   filter {}

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter {}
end


--------------------------------------------------------------------------------
-- Library: mtype-common
-- Foundation types: tokens, constants, circular dependency detection,
-- SourceLocation, and standalone string utilities (edit distance, etc.)
--------------------------------------------------------------------------------
project "mtype-common"
   kind "StaticLib"
   location "mType"
   commonConfig()

   files {
      "mType/token/**.hpp",
      "mType/token/**.cpp",
      "mType/constants/**.hpp",
      "mType/constants/**.cpp",
      "mType/circularDependency/**.hpp",
      "mType/circularDependency/**.cpp",
      "mType/util/**.hpp",
      "mType/util/**.cpp",
      "mType/errors/SourceLocation.hpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-errors
-- Error/exception types (excluding UserException which depends on core types)
--------------------------------------------------------------------------------
project "mtype-errors"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common" }

   files {
      "mType/errors/**.hpp",
      "mType/errors/**.cpp",
   }

   removefiles {
      "mType/errors/SourceLocation.hpp",
      "mType/errors/UserException.hpp",
      "mType/errors/UserException.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-diagnostics
-- Shared Diagnostic data model + renderer + cache used by both the CLI
-- and the language server. Depends on mtype-errors and mtype-core so the
-- exception converter can dynamic_cast against every exception type
-- (including UserException, whose typeinfo lives in mtype-core because
-- UserException.cpp pulls in value::Value).
--------------------------------------------------------------------------------
project "mtype-diagnostics"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core" }

   files {
      "mType/diagnostics/**.hpp",
      "mType/diagnostics/**.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-analysis
-- Compile-time analyzer passes that produce non-fatal diagnostics:
--   - OverrideAnnotationChecker (MYT-50): missing @Override warnings
--   - UnusedVariableAnalyzer    (MYT-49): unused local variable warnings
-- Depends on mtype-ast (AST traversal) + mtype-core (ClassDefinition,
-- Environment) + mtype-diagnostics (Diagnostic emission).
--------------------------------------------------------------------------------
project "mtype-analysis"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core", "mtype-ast", "mtype-diagnostics" }

   files {
      "mType/analysis/**.hpp",
      "mType/analysis/**.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-core
-- Core runtime: value types, runtime type definitions, type system, environment
-- These are merged due to circular dependencies between value/runtimeTypes/types/environment
--------------------------------------------------------------------------------
project "mtype-core"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors" }

   files {
      "mType/value/**.hpp",
      "mType/value/**.cpp",
      "mType/runtimeTypes/**.hpp",
      "mType/runtimeTypes/**.cpp",
      "mType/types/**.hpp",
      "mType/types/**.cpp",
      "mType/environment/**.hpp",
      "mType/environment/**.cpp",
      "mType/errors/UserException.hpp",
      "mType/errors/UserException.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-ast
-- Abstract Syntax Tree: all node types, visitor, generics
--------------------------------------------------------------------------------
project "mtype-ast"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core" }

   files {
      "mType/ast/**.hpp",
      "mType/ast/**.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-frontend
-- Frontend: lexer, parser, AST optimizer
--------------------------------------------------------------------------------
project "mtype-frontend"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core", "mtype-ast", "mtype-diagnostics" }

   files {
      "mType/lexer/**.hpp",
      "mType/lexer/**.cpp",
      "mType/parser/**.hpp",
      "mType/parser/**.cpp",
      "mType/optimizer/**.hpp",
      "mType/optimizer/**.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-vm
-- Virtual Machine: bytecode, compiler, runtime, optimization, event loop
-- Excludes JIT (separate library)
--------------------------------------------------------------------------------
project "mtype-vm"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core", "mtype-ast", "mtype-diagnostics", "mtype-analysis" }

   includedirs { "vendor/asmjit" }
   defines { "ASMJIT_STATIC" }

   files {
      "mType/vm/**.hpp",
      "mType/vm/**.cpp",
      "mType/runtime/**.hpp",
      "mType/runtime/**.cpp",
   }

   removefiles {
      "mType/vm/jit/**.hpp",
      "mType/vm/jit/**.cpp",
   }


--------------------------------------------------------------------------------
-- Library: mtype-jit
-- JIT compiler using asmjit
--------------------------------------------------------------------------------
project "mtype-jit"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-core", "mtype-vm" }

   includedirs { "vendor/asmjit" }
   defines { "ASMJIT_STATIC" }

   files {
      "mType/vm/jit/**.hpp",
      "mType/vm/jit/**.cpp",
      "vendor/asmjit/asmjit/**.cpp",
      "vendor/asmjit/asmjit/**.h",
   }


--------------------------------------------------------------------------------
-- Library: mtype-extensions
-- High-level features: GC, JSON, reflection, debugger, validation, project, services
--------------------------------------------------------------------------------
project "mtype-extensions"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links { "mtype-common", "mtype-errors", "mtype-core", "mtype-ast", "mtype-frontend", "mtype-vm", "mtype-diagnostics", "mtype-analysis" }

   includedirs { "packagemanager/src" }

   files {
      "mType/gc/**.hpp",
      "mType/gc/**.cpp",
      "mType/json/**.hpp",
      "mType/json/**.cpp",
      "mType/net/**.hpp",
      "mType/net/**.cpp",
      "mType/reflection/**.hpp",
      "mType/reflection/**.cpp",
      "mType/debugger/**.hpp",
      "mType/debugger/**.cpp",
      "mType/validation/**.hpp",
      "mType/validation/**.cpp",
      "mType/project/**.hpp",
      "mType/project/**.cpp",
      "mType/services/**.hpp",
      "mType/services/**.cpp",
      "packagemanager/src/**.hpp",
      "packagemanager/src/**.cpp",
   }

   filter "system:windows"
      links { "winhttp", "ws2_32" }
   filter {}

   -- Exclude standalone Main.cpp from library build
   removefiles { "packagemanager/src/Main.cpp" }


--------------------------------------------------------------------------------
-- Library: mtype-tests
-- Test framework and test suites
--------------------------------------------------------------------------------
project "mtype-tests"
   kind "StaticLib"
   location "mType"
   commonConfig()

   links {
      "mtype-common", "mtype-errors", "mtype-core", "mtype-ast",
      "mtype-frontend", "mtype-vm", "mtype-jit", "mtype-extensions",
      "mtype-diagnostics", "mtype-analysis",
   }

   includedirs { "vendor/asmjit", "packagemanager/src" }
   defines { "ASMJIT_STATIC" }

   files {
      "mType/tests/testFramework/**.hpp",
      "mType/tests/testFramework/**.cpp",
      "mType/tests/suites/**.hpp",
      "mType/tests/suites/**.cpp",
      "mType/tests/testFiles/**.mt",
      "mType/tests/testFiles/**.expected",
   }


--------------------------------------------------------------------------------
-- Executable: mType
-- Entry point that links all libraries
--------------------------------------------------------------------------------
project "mType"
   kind "ConsoleApp"
   location "mType"
   commonConfig()

   includedirs { "vendor/asmjit" }
   defines { "ASMJIT_STATIC" }

   -- Suppress LNK4006: __NULL_IMPORT_DESCRIPTOR is defined by every MSVC
   -- import library (e.g. winhttp.lib, ws2_32.lib); linking more than one
   -- triggers a benign duplicate-definition warning.
   filter "system:windows"
      linkoptions { "/ignore:4006" }
   filter {}

   files {
      "mType/run/**.hpp",
      "mType/run/**.cpp",
   }

   links {
      "mtype-tests",
      "mtype-extensions",
      "mtype-jit",
      "mtype-vm",
      "mtype-frontend",
      "mtype-ast",
      "mtype-core",
      "mtype-analysis",
      "mtype-diagnostics",
      "mtype-errors",
      "mtype-common",
   }


--------------------------------------------------------------------------------
-- Executable: mtype-launcher
-- Minimal launcher that reads appended bytecode and runs it via the VM.
-- Built alongside mType; used by `mType --build --exe` to produce
-- standalone executables with embedded bytecode.
--------------------------------------------------------------------------------
project "mtype-launcher"
   kind "ConsoleApp"
   location "mType"
   commonConfig()

   -- Output to the same directory as mType.exe so --build --exe can find it
   targetdir "bin/mType/%{cfg.buildcfg}/%{cfg.platform}"

   includedirs { "vendor/asmjit", "packagemanager/src" }
   defines { "ASMJIT_STATIC" }

   -- Suppress LNK4006: see comment on mType project above.
   filter "system:windows"
      linkoptions { "/ignore:4006" }
   filter {}

   files {
      "mType/launcher/**.hpp",
      "mType/launcher/**.cpp",
   }

   links {
      "mtype-extensions",
      "mtype-jit",
      "mtype-vm",
      "mtype-frontend",
      "mtype-ast",
      "mtype-core",
      "mtype-analysis",
      "mtype-diagnostics",
      "mtype-errors",
      "mtype-common",
   }
