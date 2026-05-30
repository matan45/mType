#include "ScriptInterpreter.hpp"
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <memory>
#include "../errors/ParseException.hpp"

#include "ImportManager.hpp"
#include "OptimizationService.hpp"
#include "NativeFunctionRegistry.hpp"
#include "ImportResolver.hpp"
#include "BytecodeService.hpp"
#include "ScriptAPI.hpp"
#include "ExecutionStrategy.hpp"
#include "BytecodeExecutionStrategy.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../net/NetNatives.hpp"
#include "../project/mtclib/LibraryNatives.hpp"
#include "../project/ProjectConfigParser.hpp"
#include "MtModulesManager.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../analysis/UnusedVariableAnalyzer.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"

namespace services
{
    ScriptInterpreter::ScriptInterpreter()
        : executionMode(constants::ExecutionMode::BYTECODE_VM)
    {
        initializeServices();
    }

    ScriptInterpreter::ScriptInterpreter(constants::ExecutionMode mode)
        : executionMode(mode)
    {
        initializeServices();
    }

    void ScriptInterpreter::initializeServices()
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        optimizationService = std::make_unique<OptimizationService>();
        nativeRegistry = std::make_unique<NativeFunctionRegistry>(environment);
        importResolver = std::make_unique<ImportResolver>(environment);
        // Always skip strict validation since optimizations may have removed unused methods.
        compiler = std::make_unique<vm::compiler::BytecodeCompiler>(environment, true);
        vm = std::make_shared<vm::runtime::VirtualMachine>(environment);

        // ScriptAPI is initialized with VM support; program is set later when bytecode is loaded.
        scriptAPI = std::make_unique<ScriptAPI>(environment, vm.get(), nullptr);

        // BytecodeService needs the ScriptAPI reference so it can update program during execution.
        bytecodeService = std::make_unique<BytecodeService>(environment, optimizationService.get(), vm, scriptAPI.get());

        executionStrategy = std::make_unique<BytecodeExecutionStrategy>(compiler.get(), vm, importResolver.get(), scriptAPI.get());

        reflection::ReflectionNatives::registerAll(environment);
        net::NetNatives::registerAll(environment);

        transitiveDependencyLoader = std::make_shared<project::mtclib::TransitiveDependencyLoader>();
        project::mtclib::LibraryNatives::registerAll(environment, transitiveDependencyLoader);
    }

    ScriptInterpreter::~ScriptInterpreter()
    {
        net::NetNatives::cleanup();
        project::mtclib::LibraryNatives::cleanup();

        cleanupRegistries();
    }

    void ScriptInterpreter::applyImportConfig(
        const std::vector<std::string>& searchPaths,
        const std::unordered_map<std::string, std::string>& aliases,
        const std::string& projectRoot)
    {
        pendingSearchPaths_ = searchPaths;
        pendingAliases_ = aliases;
        pendingProjectRoot_ = projectRoot;
    }

    bool ScriptInterpreter::tryLoadAmbientProject(const std::string& scriptPath)
    {
        try
        {
            std::filesystem::path path(scriptPath);
            std::string scriptDir = path.has_parent_path()
                ? path.parent_path().string()
                : std::string(".");

            project::ProjectConfigParser projParser;
            auto mtprojPath = projParser.findProject(scriptDir);
            if (!mtprojPath.has_value()) return false;

            auto config = projParser.parse(*mtprojPath);

            // Mirror ProjectBuilder::buildMergedAliases precedence
            // (mt_modules first, <Alias> entries override on collision).
            // Inlined because buildMergedAliases is a private member.
            std::unordered_map<std::string, std::string> merged;
            try
            {
                packagemanager::MtModulesManager modulesMgr(config->projectRoot);
                for (const auto& [name, p] : modulesMgr.getAliases())
                {
                    merged[name] = p;
                }
            }
            catch (const std::exception&)
            {
                // A broken mt_modules/ must not block scripts that don't
                // actually depend on installed packages.
            }
            for (const auto& [name, p] : config->imports.aliases)
            {
                merged[name] = p;
            }

            applyImportConfig(config->imports.searchPaths,
                              merged,
                              config->projectRoot);
            return true;
        }
        catch (const std::exception& e)
        {
            // A broken .mtproj must not break direct execution of files
            // that don't actually depend on its aliases.
            std::cerr << "[mType] Warning: failed to load ambient .mtproj: "
                      << e.what() << "\n";
            return false;
        }
    }

    void ScriptInterpreter::runScript(const std::string& filename)
    {
        try
        {
            auto [ast, importManager] = parseScriptFile(filename);

            environment->setImportManager(importManager.get());

            value::Value result = executeScriptAST(std::move(ast));

            // Automatic cleanup after script execution to prevent memory growth.
            // Only cleans up interfaces that are no longer referenced.
            cleanupUnusedInterfaces();

            // ImportManager destructs here when it goes out of scope.
        }
        catch (const ParseException&)
        {
            // Re-throw to be caught by main() for centralized error handling;
            // don't print here to avoid duplicate error messages.
            throw;
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    std::pair<std::unique_ptr<ast::ASTNode>, std::unique_ptr<ImportManager>>
    ScriptInterpreter::parseScriptFile(const std::string& filename)
    {
        lexer::Lexer lexer(filename);

        auto importManager = std::make_unique<ImportManager>();

        std::filesystem::path scriptPath(filename);
        importManager->setBaseDirectory(scriptPath.parent_path().string());

        // MYT-310 — if Main.cpp discovered an ambient .mtproj and called
        // applyImportConfig, propagate its aliases / search paths / project
        // root into the per-script ImportManager so `@pkg/...` resolves the
        // same way it does under --build / --compile. Project root override
        // also enforces security containment via ImportManager::setProjectRoot.
        if (!pendingProjectRoot_.empty())
        {
            importManager->setBaseDirectory(pendingProjectRoot_);
            importManager->setProjectRoot(pendingProjectRoot_);
        }
        if (!pendingSearchPaths_.empty())
        {
            importManager->setSearchPaths(pendingSearchPaths_);
        }
        if (!pendingAliases_.empty())
        {
            importManager->setPathAliases(pendingAliases_);
        }

        // Anchor imports in the main file to its own resolved location.
        std::string canonicalPath = std::filesystem::canonical(filename).string();
        importManager->setCurrentFilePath(canonicalPath);

        parser::Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Take the ImportManager back from Parser before Parser is destroyed.
        auto retrievedImportManager = parser.getImportManager();

        return {std::move(ast), std::move(retrievedImportManager)};
    }

    value::Value ScriptInterpreter::executeScriptAST(std::unique_ptr<ast::ASTNode> ast)
    {
        // IMPORTANT: resolve all imports BEFORE optimization, so the
        // optimizer sees imported code.
        importResolver->resolveImports(ast.get());

        // MYT-49 — run the unused-variable analyzer against the resolved
        // AST BEFORE optimization (so the analyzer sees the user's
        // original code shape, not the optimized form). Push results
        // into the BytecodeCompiler's warning sink so Main.cpp's
        // post-runScript loop renders them.
        if (compiler)
        {
            auto unusedWarnings = analysis::UnusedVariableAnalyzer::analyze(ast.get());
            for (auto& w : unusedWarnings)
            {
                compiler->addWarning(std::move(w));
            }
        }

        ast = optimizationService->applyOptimizations(std::move(ast), environment);

        return executionStrategy->execute(ast.get());
    }

    void ScriptInterpreter::resetForRebuild()
    {
        if (environment)
        {
            environment->resetForRebuild();
        }

        cachedBytecodeProgram.reset();

        if (vm)
        {
            vm->reset();
        }

        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(nullptr);
        }
    }

    void ScriptInterpreter::cleanupRegistries()
    {
        if (environment)
        {
            environment->cleanupUnusedInterfaces();
            auto interfaceRegistry = environment->getInterfaceRegistry();
            if (interfaceRegistry)
            {
                interfaceRegistry->clearValidationCache();
            }
        }
    }

    size_t ScriptInterpreter::cleanupUnusedInterfaces()
    {
        if (!environment) return 0;
        return environment->cleanupUnusedInterfaces();
    }

    std::vector<std::string> ScriptInterpreter::findUnusedInterfaces() const
    {
        if (!environment) return {};
        return environment->findUnusedInterfaces();
    }

    void ScriptInterpreter::parseAndRegisterClasses(const std::string& filename)
    {
        try
        {
            // Pre-compiled bytecode and .mtcLib containers go through the
            // loader path; everything else is parsed from source.
            const bool isMtc = filename.length() >= 4 && filename.substr(filename.length() - 4) == ".mtc";
            const bool isMtcLib = filename.length() >= 7 && filename.substr(filename.length() - 7) == ".mtcLib";
            if (isMtc || isMtcLib)
            {
                loadCompiledBytecode(filename);
                return;
            }

            auto [ast, importManager] = parseScriptFile(filename);
            environment->setImportManager(importManager.get());

            // IMPORTANT: resolve all imports BEFORE compilation so
            // imported classes are included in the bytecode.
            importResolver->resolveImports(ast.get());

            // Compile the AST to bytecode — classes are registered as a
            // side effect by ClassRegistrar. We deliberately don't execute,
            // so any top-level code in the script doesn't run.
            if (compiler)
            {
                cachedBytecodeProgram = std::make_unique<vm::bytecode::BytecodeProgram>(compiler->compile(ast.get()));

                // Wire program reference into VM + ScriptAPI for later C++
                // API calls (createObject, invokeMethod, etc.).
                if (vm)
                {
                    vm->setProgram(cachedBytecodeProgram.get());
                }
                if (scriptAPI)
                {
                    scriptAPI->setBytecodeProgram(cachedBytecodeProgram.get());
                }
                runCachedStaticInitializers();
            }
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception&)
        {
            throw;
        }
    }

    void ScriptInterpreter::setExecutionMode(constants::ExecutionMode mode)
    {
        executionMode = mode;
    }

    constants::ExecutionMode ScriptInterpreter::getExecutionMode() const
    {
        return executionMode;
    }

    std::shared_ptr<environment::Environment> ScriptInterpreter::getEnvironment() const
    {
        return environment;
    }

    const std::vector<diagnostics::Diagnostic>& ScriptInterpreter::getCompilerWarnings() const
    {
        // The compiler is built lazily by initializeServices(); if a caller
        // asks for warnings before any compile has happened, fall back to a
        // shared empty vector so the reference stays valid.
        static const std::vector<diagnostics::Diagnostic> kEmpty;
        return compiler ? compiler->warnings() : kEmpty;
    }

    void ScriptInterpreter::enableDebugging()
    {
        if (vm)
        {
            vm->setDebuggingEnabled(true);
        }
    }

    void ScriptInterpreter::disableDebugging()
    {
        if (vm)
        {
            vm->setDebuggingEnabled(false);
        }
    }

    void ScriptInterpreter::setCurrentBytecodeProgram(const vm::bytecode::BytecodeProgram* program)
    {
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(program);
        }
    }
}
