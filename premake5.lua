workspace "Interpreter"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "."  -- Place generated solution files in the root directory
   startproject "mType"
   -- toolset is set per-platform in commonConfig() below


-- Set a consistent location for build artifacts
builddir = "bin/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"

-- Shared configuration applied to all projects
function commonConfig()
   language "C++"
   cppdialect "C++20"
   targetdir (builddir)

   defines { "_CRT_SECURE_NO_WARNINGS", "MTYPE_SIMD_ENABLED" }

   -- Platform-specific configurations
   filter { "system:windows", "action:vs*" }
      toolset "v145"
      systemversion "latest"
      buildoptions { "/MP" }

   filter { "system:windows", "configurations:Release" }
      buildoptions { "/arch:AVX2" }

   filter "system:linux or system:macosx"
      buildoptions { "-msse2", "-msse4.1" }
      links { "pthread" }

   -- GNU ld does single-pass static-lib resolution; mtype-vm <-> mtype-jit and
   -- mtype-core <-> mtype-extensions/-ast have cyclic refs that MSVC/ld64
   -- handle natively but ld does not. Wrap archives in --start-group.
   filter "system:linux"
      linkgroups "On"

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
      "mType/plugin/**.h",
      "mType/plugin/**.hpp",
      "mType/plugin/**.cpp",
      "packagemanager/src/**.hpp",
      "packagemanager/src/**.cpp",
   }

   -- NOTE: Windows system libs (winhttp, ws2_32) are intentionally NOT linked
   -- here even though this project uses them. When listed on a StaticLib,
   -- premake emits them into <Lib><AdditionalDependencies>, which causes
   -- lib.exe to merge the import libs into mtype-extensions.lib and triggers
   -- a non-suppressible LNK4006 on __NULL_IMPORT_DESCRIPTOR. The libs are
   -- instead linked by the consuming exe projects (mType, mtype-launcher,
   -- mtype-launcher-gui), where /IGNORE:4006 on the final link step is honored.

   -- On Linux/macOS use libcurl for HTTP, libdl for plugin loading, and
   -- exclude Win-only files.
   filter "system:linux or system:macosx"
      links { "curl", "dl" }
      removefiles {
         "mType/net/WinHttpClient.hpp",
         "mType/net/WinHttpClient.cpp",
         "mType/net/WinSocket.hpp",
         "mType/net/WinSocket.cpp",
         "mType/net/WinSockInit.hpp",
         "mType/net/WinSockInit.cpp",
         "mType/plugin/WinPluginLoader.cpp",
      }

   -- On Windows exclude POSIX-only files.
   filter "system:windows"
      removefiles {
         "mType/net/CurlHttpClient.hpp",
         "mType/net/CurlHttpClient.cpp",
         "mType/net/PosixSocket.hpp",
         "mType/net/PosixSocket.cpp",
         "mType/plugin/PosixPluginLoader.cpp",
      }
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

   -- Link Windows system libs here (not in mtype-extensions) so lib.exe
   -- never merges two import libraries — that merge is what triggers
   -- LNK4006 on __NULL_IMPORT_DESCRIPTOR. /IGNORE:4006 covers the residual
   -- case at the link step.
   filter "system:windows"
      links { "winhttp", "ws2_32" }
      linkoptions { "/ignore:4006" }
   filter "system:linux or system:macosx"
      links { "curl" }
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

   -- Build the plugin-test-fixture shared lib whenever mType.exe is built so
   -- PluginTestSuite's integration test (MYT-289) can dlopen it. Build-only
   -- dep — the fixture is loaded at runtime, never linked.
   dependson { "plugin-test-fixture" }


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

   -- See comment on mType project above.
   filter "system:windows"
      links { "winhttp", "ws2_32" }
      linkoptions { "/ignore:4006" }
   filter "system:linux or system:macosx"
      links { "curl" }
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


--------------------------------------------------------------------------------
-- Executable: mtype-launcher-gui (MYT-326)
-- Windowed-subsystem variant of mtype-launcher, selected by
-- `mType --build --exe --gui`. On Windows /SUBSYSTEM:WINDOWS suppresses the
-- auto-allocated console window when the produced exe is double-clicked from
-- Explorer; /ENTRY:mainCRTStartup keeps the shared `int main()` in
-- Launcher.cpp as the entry point (no separate WinMain needed).
-- On Linux/macOS there's no subsystem concept; this builds as a regular exe
-- identical in behavior to mtype-launcher (alias).
--------------------------------------------------------------------------------
project "mtype-launcher-gui"
   kind "WindowedApp"
   location "mType"
   commonConfig()

   targetdir "bin/mType/%{cfg.buildcfg}/%{cfg.platform}"

   includedirs { "vendor/asmjit", "packagemanager/src" }
   defines { "ASMJIT_STATIC" }

   filter "system:windows"
      links { "winhttp", "ws2_32" }
      linkoptions { "/ignore:4006", "/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup" }
   filter "system:linux or system:macosx"
      links { "curl" }
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


--------------------------------------------------------------------------------
-- Executable: mtpm
-- Standalone package-manager CLI.
--------------------------------------------------------------------------------
project "mtpm"
   kind "ConsoleApp"
   location "packagemanager"
   commonConfig()

   includedirs {
      "packagemanager/src",
      "mType",
   }

   files {
      "packagemanager/src/**.hpp",
      "packagemanager/src/**.cpp",
      "mType/project/ProjectConfigParser.hpp",
      "mType/project/ProjectConfigParser.cpp",
      "mType/project/ProjectConfig.hpp",
      "mType/project/XmlParser.hpp",
      "mType/project/XmlParser.cpp",
      "mType/project/GlobMatcher.hpp",
      "mType/project/GlobMatcher.cpp",
      "mType/services/FileReader.hpp",
      "mType/services/FileReader.cpp",
      "mType/json/JsonParser.hpp",
      "mType/json/JsonParser.cpp",
      "mType/json/JsonValue.hpp",
      "mType/json/JsonValue.cpp",
   }


--------------------------------------------------------------------------------
-- Language Server shared configuration
--------------------------------------------------------------------------------
function lspCommonConfig()
   commonConfig()

   includedirs {
      "languageserver/src",
      "languageserver/vendor",
      "mType",
      "packagemanager/src",
   }

   -- The LSP path parses and analyzes source but does not execute bytecode,
   -- so diagnostics can exclude UserException and avoid linking runtime value
   -- machinery into the language-server library.
   defines { "MTYPE_DIAGNOSTICS_NO_USER_EXCEPTION" }
end


--------------------------------------------------------------------------------
-- Library: mtype-language-server-lib
-- LSP handler/analysis code plus the minimal mType frontend/core sources it uses.
--------------------------------------------------------------------------------
project "mtype-language-server-lib"
   kind "StaticLib"
   location "languageserver"
   lspCommonConfig()

   files {
      "languageserver/src/**.hpp",
      "languageserver/src/**.cpp",
      "languageserver/vendor/**.hpp",

      "mType/diagnostics/**.hpp",
      "mType/diagnostics/**.cpp",
      "mType/analysis/**.hpp",
      "mType/analysis/**.cpp",
      "mType/util/**.hpp",
      "mType/util/**.cpp",
      "mType/errors/AccessViolationException.cpp",

      "mType/lexer/Lexer.cpp",
      "mType/lexer/BracketBalancer.cpp",
      "mType/lexer/SourceLocationTracker.cpp",
      "mType/lexer/TokenFactory.cpp",

      "mType/parser/Parser.cpp",
      "mType/parser/AnnotationDeclarationParser.cpp",
      "mType/parser/ClassParser.cpp",
      "mType/parser/ExpressionParser.cpp",
      "mType/parser/InterfaceParser.cpp",
      "mType/parser/LambdaParser.cpp",
      "mType/parser/StatementParser.cpp",
      "mType/parser/TypeParser.cpp",
      "mType/parser/ParseContext.cpp",
      "mType/parser/ParserValidator.cpp",
      "mType/parser/TokenStream.cpp",
      "mType/parser/TypeRegistry.cpp",
      "mType/parser/core/BaseParser.cpp",
      "mType/parser/class/GenericParameterParser.cpp",
      "mType/parser/utilities/StatementTypeDetector.cpp",
      "mType/parser/utilities/VisibilityParser.cpp",
      "mType/parser/utilities/AccessModifierParser.cpp",
      "mType/parser/utilities/NameValidator.cpp",
      "mType/parser/utilities/AsyncValidator.cpp",
      "mType/parser/utilities/ParameterParser.cpp",
      "mType/parser/utilities/QualifiedNameParser.cpp",
      "mType/parser/utilities/ParserUtils.cpp",
      "mType/parser/utilities/AnnotationParser.cpp",

      "mType/ast/ASTNode.cpp",
      "mType/ast/AccessModifier.cpp",
      "mType/ast/GenericType.cpp",
      "mType/ast/GenericTypeParameter.cpp",
      "mType/ast/GenericTypeSubstitutionContext.cpp",
      "mType/ast/nodes/**/*.cpp",
      "mType/ast/utils/GenericTypeConversionUtils.cpp",

      "mType/environment/Environment.cpp",
      "mType/environment/EnvironmentBuilder.cpp",
      "mType/environment/manager/*.cpp",
      "mType/environment/registry/ClassRegistry.cpp",
      "mType/environment/registry/ExportRegistry.cpp",
      "mType/environment/registry/FunctionRegistry.cpp",
      "mType/environment/registry/InheritanceTracker.cpp",
      "mType/environment/registry/TypeCatalog.cpp",

      "mType/services/FileReader.cpp",
      "mType/services/ImportManager.cpp",

      -- MYT-309: pulled in so the LSP can resolve `@pkg/...` imports
      -- via mt_modules/ — same scan the runtime uses through
      -- ProjectBuilder::buildMergedAliases.
      "packagemanager/src/MtModulesManager.hpp",
      "packagemanager/src/MtModulesManager.cpp",
      "packagemanager/src/PackageManifest.hpp",
      "packagemanager/src/PackageManifest.cpp",
      "mType/json/JsonParser.hpp",
      "mType/json/JsonParser.cpp",
      "mType/json/JsonValue.hpp",
      "mType/json/JsonValue.cpp",

      "mType/types/TypeConversionUtils.cpp",
      "mType/types/TypeDescriptors.cpp",
      "mType/types/UnifiedType.cpp",
      "mType/types/ReifiedTypeRegistry.cpp",
      "mType/types/TypeSubstitutionService.cpp",

      "mType/vm/MethodSignature.cpp",

      "mType/environment/registry/FunctionDefinition.cpp",
      "mType/environment/registry/VariableDefinition.cpp",
      "mType/environment/registry/ClassDefinition.cpp",
      "mType/environment/registry/ClassDefinition_Fields.cpp",
      "mType/environment/registry/ClassDefinition_Methods.cpp",
      "mType/environment/registry/ClassDefinition_Hierarchy.cpp",
      "mType/environment/registry/ClassDefinition_Interfaces.cpp",
      "mType/environment/registry/ConstructorDefinition.cpp",
      "mType/environment/registry/FieldDefinition.cpp",
      "mType/environment/registry/InterfaceDefinition.cpp",
      "mType/environment/registry/InterfaceRegistry.cpp",
      "mType/environment/registry/MethodDefinition.cpp",
      "mType/environment/registry/SignatureUtils.cpp",

      "mType/parser/AnnotationDeclarationParser.cpp",
      "mType/validation/builtins/BuiltInAnnotations.cpp",
      "mType/project/mtclib/MtcPathResolver.cpp",

      "mType/circularDependency/CircularDependencyDetector.cpp",
      "mType/circularDependency/DependencyPatternAnalyzer.cpp",
      "mType/circularDependency/DependencyTypeUtils.cpp",

      "mType/value/StringPool.cpp",
      "mType/value/InternedString.cpp",
      "mType/value/ValueTypeUtils.cpp",
   }


--------------------------------------------------------------------------------
-- Executable: mtype-language-server
--------------------------------------------------------------------------------
project "mtype-language-server"
   kind "ConsoleApp"
   location "languageserver"
   lspCommonConfig()

   files { "languageserver/main.cpp" }

   links { "mtype-language-server-lib" }


--------------------------------------------------------------------------------
-- Executable: mtype-language-server-tests
--------------------------------------------------------------------------------
project "mtype-language-server-tests"
   kind "ConsoleApp"
   location "languageserver"
   lspCommonConfig()

   includedirs { "languageserver/tests" }

   files {
      "languageserver/tests/**.hpp",
      "languageserver/tests/**.cpp",
   }

   links { "mtype-language-server-lib" }


--------------------------------------------------------------------------------
-- Shared library: plugin-test-fixture (MYT-289)
-- Built independently of the engine to validate the C-ABI plugin boundary
-- in CI. Outputs plugin_test_fixture.dll/.so/.dylib at a stable path so
-- PluginTestSuite's integration test can dlopen it via the real loader.
--
-- IMPORTANT: this project links NOTHING from the engine. Its only build
-- dependency is mType/plugin/PluginHostApi.h — the same surface a third
-- party would consume. If you find yourself adding `links { "mtype-..." }`
-- here, stop: it would defeat the point of the fixture.
--------------------------------------------------------------------------------
project "plugin-test-fixture"
   kind "SharedLib"
   location "mType"
   commonConfig()

   -- Override commonConfig's targetdir so the binary lands at a stable,
   -- config-independent location PluginTestSuite can find with a single
   -- relative path (CWD = project root when running `mType.exe --tests`).
   targetdir   "mType/tests/testFiles/plugin"
   targetname  "plugin_test_fixture"
   targetprefix ""  -- no "lib" prefix on POSIX

   includedirs { "mType/plugin" }

   files {
      "mType/tests/pluginFixture/fixture.cpp",
   }

   filter "system:linux or system:macosx"
      buildoptions { "-fvisibility=hidden" }
   filter {}


-- The SDL3 + ImGui plugin previously lived here. It moved to its own
-- standalone repo at C:\matan\mtype-imgui-sdl (CMake build, not premake)
-- so third-party plugins follow the recommended out-of-tree pattern.
-- See https://github.com/matan45/mType — wiki / native-interop docs for
-- where to find it.
