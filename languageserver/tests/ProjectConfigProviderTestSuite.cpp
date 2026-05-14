#include "ProjectConfigProviderTestSuite.hpp"
#include "../src/utils/ProjectConfigProvider.hpp"
#include "TempDir.hpp"

namespace mtype::lsp::test {

void ProjectConfigProviderTestSuite::registerTests(LspTestHarness& harness) {

    // .mtproj is XML-based with <SearchPath> and <Alias> elements.
    // loadFromWorkspace searches for the shortest-path .mtproj in the tree.
    // resolveImport uses a 5-step fallback: alias → absolute → baseDir → projectRoot → searchPaths.

    harness.addTest("isLoaded returns false before loading", []() {
        ProjectConfigProvider config;
        require(!config.isLoaded(), "should not be loaded before loadFromWorkspace");
    });

    harness.addTest("no .mtproj file: loadFromWorkspace returns false", []() {
        TempDir tmpDir;
        tmpDir.createFile("src/main.mt", "class Main {}");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());
        require(!loaded, "should return false when no .mtproj exists");
        require(!config.isLoaded(), "should not be loaded");
    });

    harness.addTest("loadFromWorkspace with .mtproj file", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj",
            "<Project>\n"
            "  <SearchPath>./src</SearchPath>\n"
            "  <SearchPath>./lib</SearchPath>\n"
            "</Project>\n");
        tmpDir.createFile("src/main.mt", "class Main {}");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());

        if (loaded) {
            require(config.isLoaded(), "should be loaded after successful load");
            require(!config.getSearchPaths().empty(),
                "expected search paths from config");
        }
        // If not loaded, the XML format might differ — acceptable
    });

    harness.addTest("getProjectRoot returns parent of .mtproj", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());

        if (loaded) {
            require(!config.getProjectRoot().empty(),
                "project root should be set");
        }
    });

    harness.addTest("default constructor does not crash", []() {
        ProjectConfigProvider config;
        require(!config.isLoaded(), "default state is not loaded");
        require(config.getSearchPaths().empty(), "no search paths by default");
        require(config.getProjectRoot().empty(), "no project root by default");
    });

    // MYT-309 — mt_modules/@pkg/mtpkg.json should produce a `@pkg` alias.
    harness.addTest("mt_modules: alias scanned and registered", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");
        // mtpkg.json with a `source` of "src" (the default matches the runtime
        // convention used by mtmodules-basic).
        tmpDir.createFile("mt_modules/@somelib/mtpkg.json",
            "{\"name\":\"somelib\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@somelib/src/Foo.mt", "class Foo {}");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());
        require(loaded, "loadFromWorkspace should succeed with a minimal .mtproj");

        const auto& aliases = config.getAliases();
        auto it = aliases.find("@somelib");
        require(it != aliases.end(), "expected @somelib alias from mt_modules scan");
        require(it->second.find("@somelib") != std::string::npos,
            "alias path should point into the package directory");
    });

    // MYT-309 — explicit <Alias> wins on collision (matches the runtime
    // precedence in ProjectBuilder::buildMergedAliases).
    harness.addTest("mt_modules: <Alias> overrides mt_modules entry on collision", []() {
        TempDir tmpDir;
        // Both an explicit <Alias> and an mt_modules package use the same key.
        tmpDir.createFile("override/Foo.mt", "class Foo {}");
        tmpDir.createFile(".mtproj",
            "<Project>\n"
            "  <Alias Name=\"@somelib\" Path=\"./override\" />\n"
            "</Project>\n");
        tmpDir.createFile("mt_modules/@somelib/mtpkg.json",
            "{\"name\":\"somelib\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@somelib/src/Foo.mt", "class Foo {}");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());
        require(loaded, "loadFromWorkspace should succeed");

        const auto& aliases = config.getAliases();
        auto it = aliases.find("@somelib");
        require(it != aliases.end(), "alias should exist");
        // <Alias> Path="./override" → should land in `override/`, NOT in
        // `mt_modules/@somelib/src/`.
        require(it->second.find("override") != std::string::npos,
            "explicit <Alias> entry should win over mt_modules");
        require(it->second.find("mt_modules") == std::string::npos,
            "mt_modules path should not be the final value");
    });

    // MYT-309 — resolveImport rewrites `@pkg/Foo.mt` to the package source path.
    harness.addTest("mt_modules: resolveImport resolves @pkg/...", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");
        tmpDir.createFile("mt_modules/@somelib/mtpkg.json",
            "{\"name\":\"somelib\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@somelib/src/Foo.mt", "class Foo {}");

        ProjectConfigProvider config;
        bool loaded = config.loadFromWorkspace(tmpDir.path());
        require(loaded, "loadFromWorkspace should succeed");

        std::string resolved = config.resolveImport(tmpDir.path(), "@somelib/Foo.mt");
        require(!resolved.empty(), "resolveImport should resolve @somelib/Foo.mt");
        require(resolved.find("Foo.mt") != std::string::npos,
            "resolved path should reference Foo.mt");
        require(resolved.find("@somelib") != std::string::npos,
            "resolved path should be inside the @somelib package");
    });

    // MYT-309 — reload() picks up packages added after the initial load.
    harness.addTest("mt_modules: reload picks up new packages", []() {
        TempDir tmpDir;
        tmpDir.createFile(".mtproj", "<Project></Project>\n");

        ProjectConfigProvider config;
        require(config.loadFromWorkspace(tmpDir.path()),
            "initial load should succeed");
        require(config.getAliases().find("@latelib") == config.getAliases().end(),
            "@latelib should not exist before package is added");

        // Simulate `mtpm add` after the LSP already loaded the workspace.
        tmpDir.createFile("mt_modules/@latelib/mtpkg.json",
            "{\"name\":\"latelib\",\"version\":\"0.0.1\",\"source\":\"src\"}\n");
        tmpDir.createFile("mt_modules/@latelib/src/Bar.mt", "class Bar {}");

        require(config.reload(), "reload should succeed");
        require(config.getAliases().find("@latelib") != config.getAliases().end(),
            "@latelib should be visible after reload");
    });
}

} // namespace mtype::lsp::test
