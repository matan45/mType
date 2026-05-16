#include "PackageManagerTestSuite.hpp"
#include "SemVer.hpp"
#include "PackageManifest.hpp"
#include "Lockfile.hpp"
#include "PackageRegistry.hpp"
#include "DependencyResolver.hpp"
#include "PackageInstaller.hpp"
#include "PackageCompiler.hpp"
#include "MtModulesManager.hpp"
#include "Publish.hpp"
#include "Sha256.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tests::testSuite
{
    using namespace testFramework;
    namespace fs = std::filesystem;

    void PackageManagerTestSuite::setupTests()
    {
        // =============================================
        // SemVer Parsing Tests
        // =============================================

        addCallbackTest("SemVer: parse valid version", "", [](services::ScriptAPI&) {
            auto ver = packagemanager::SemVer::parse("1.2.3");
            if (ver.major != 1 || ver.minor != 2 || ver.patch != 3)
                throw std::runtime_error("Expected 1.2.3, got " + ver.toString());
        });

        addCallbackTest("SemVer: parse zero version", "", [](services::ScriptAPI&) {
            auto ver = packagemanager::SemVer::parse("0.0.0");
            if (ver.major != 0 || ver.minor != 0 || ver.patch != 0)
                throw std::runtime_error("Expected 0.0.0, got " + ver.toString());
        });

        addCallbackTest("SemVer: parse rejects invalid format", "", [](services::ScriptAPI&) {
            try
            {
                packagemanager::SemVer::parse("1.2");
                throw std::runtime_error("Expected exception for invalid semver");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("Invalid") == std::string::npos && msg.find("invalid") == std::string::npos
                    && msg.find("stoi") == std::string::npos)
                    throw;
            }
        });

        addCallbackTest("SemVer: toString round-trip", "", [](services::ScriptAPI&) {
            auto ver = packagemanager::SemVer::parse("10.20.30");
            if (ver.toString() != "10.20.30")
                throw std::runtime_error("Expected '10.20.30', got '" + ver.toString() + "'");
        });

        // =============================================
        // SemVer Comparison Tests
        // =============================================

        addCallbackTest("SemVer: comparison operators", "", [](services::ScriptAPI&) {
            auto v1 = packagemanager::SemVer::parse("1.0.0");
            auto v2 = packagemanager::SemVer::parse("1.2.0");
            auto v3 = packagemanager::SemVer::parse("2.0.0");
            auto v1dup = packagemanager::SemVer::parse("1.0.0");

            if (!(v1 < v2)) throw std::runtime_error("1.0.0 should be < 1.2.0");
            if (!(v2 < v3)) throw std::runtime_error("1.2.0 should be < 2.0.0");
            if (!(v1 == v1dup)) throw std::runtime_error("1.0.0 should == 1.0.0");
            if (!(v3 > v1)) throw std::runtime_error("2.0.0 should be > 1.0.0");
            if (!(v1 <= v2)) throw std::runtime_error("1.0.0 should be <= 1.2.0");
            if (!(v1 >= v1dup)) throw std::runtime_error("1.0.0 should be >= 1.0.0");
            if (v1 != v1dup) throw std::runtime_error("1.0.0 should not != 1.0.0");
        });

        addCallbackTest("SemVer: patch comparison", "", [](services::ScriptAPI&) {
            auto v1 = packagemanager::SemVer::parse("1.2.3");
            auto v2 = packagemanager::SemVer::parse("1.2.4");
            if (!(v1 < v2)) throw std::runtime_error("1.2.3 should be < 1.2.4");
        });

        // =============================================
        // SemVer Range Tests
        // =============================================

        addCallbackTest("SemVer: caret range ^1.2.0", "", [](services::ScriptAPI&) {
            auto range = packagemanager::SemVerRange::parse("^1.2.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.2.0")))
                throw std::runtime_error("^1.2.0 should satisfy 1.2.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.9.9")))
                throw std::runtime_error("^1.2.0 should satisfy 1.9.9");
            if (range.satisfiedBy(packagemanager::SemVer::parse("2.0.0")))
                throw std::runtime_error("^1.2.0 should NOT satisfy 2.0.0");
            if (range.satisfiedBy(packagemanager::SemVer::parse("1.1.9")))
                throw std::runtime_error("^1.2.0 should NOT satisfy 1.1.9");
        });

        addCallbackTest("SemVer: tilde range ~1.2.0", "", [](services::ScriptAPI&) {
            auto range = packagemanager::SemVerRange::parse("~1.2.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.2.0")))
                throw std::runtime_error("~1.2.0 should satisfy 1.2.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.2.9")))
                throw std::runtime_error("~1.2.0 should satisfy 1.2.9");
            if (range.satisfiedBy(packagemanager::SemVer::parse("1.3.0")))
                throw std::runtime_error("~1.2.0 should NOT satisfy 1.3.0");
        });

        addCallbackTest("SemVer: exact version", "", [](services::ScriptAPI&) {
            auto range = packagemanager::SemVerRange::parse("1.2.3");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.2.3")))
                throw std::runtime_error("exact 1.2.3 should satisfy 1.2.3");
            if (range.satisfiedBy(packagemanager::SemVer::parse("1.2.4")))
                throw std::runtime_error("exact 1.2.3 should NOT satisfy 1.2.4");
        });

        addCallbackTest("SemVer: explicit range >=1.0.0 <2.0.0", "", [](services::ScriptAPI&) {
            auto range = packagemanager::SemVerRange::parse(">=1.0.0 <2.0.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.0.0")))
                throw std::runtime_error("should satisfy 1.0.0");
            if (!range.satisfiedBy(packagemanager::SemVer::parse("1.9.9")))
                throw std::runtime_error("should satisfy 1.9.9");
            if (range.satisfiedBy(packagemanager::SemVer::parse("2.0.0")))
                throw std::runtime_error("should NOT satisfy 2.0.0");
            if (range.satisfiedBy(packagemanager::SemVer::parse("0.9.9")))
                throw std::runtime_error("should NOT satisfy 0.9.9");
        });

        addCallbackTest("SemVer: findBestMatch picks highest", "", [](services::ScriptAPI&) {
            std::vector<packagemanager::SemVer> versions = {
                packagemanager::SemVer::parse("1.0.0"),
                packagemanager::SemVer::parse("1.2.0"),
                packagemanager::SemVer::parse("1.5.0"),
                packagemanager::SemVer::parse("2.0.0"),
            };
            auto range = packagemanager::SemVerRange::parse("^1.0.0");
            auto best = packagemanager::findBestMatch(versions, range);
            if (best.toString() != "1.5.0")
                throw std::runtime_error("Expected 1.5.0, got " + best.toString());
        });

        addCallbackTest("SemVer: findBestMatch throws when no match", "", [](services::ScriptAPI&) {
            std::vector<packagemanager::SemVer> versions = {
                packagemanager::SemVer::parse("1.0.0"),
            };
            auto range = packagemanager::SemVerRange::parse("^2.0.0");
            try
            {
                packagemanager::findBestMatch(versions, range);
                throw std::runtime_error("Expected exception for no match");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("No version") == std::string::npos)
                    throw;
            }
        });

        // =============================================
        // PackageManifest Tests
        // =============================================

        addCallbackTest("PackageManifest: parse mtpkg.json", "", [](services::ScriptAPI&) {
            std::string json = R"({
                "name": "testpkg",
                "version": "1.0.0",
                "description": "A test package",
                "source": "src",
                "library": "build/testpkg.mtcLib",
                "dependencies": { "dep1": "^1.0.0" }
            })";

            auto manifest = packagemanager::PackageManifest::parseFromJson(json);
            if (manifest.name != "testpkg")
                throw std::runtime_error("Expected name 'testpkg', got '" + manifest.name + "'");
            if (manifest.version != "1.0.0")
                throw std::runtime_error("Expected version '1.0.0'");
            if (manifest.description != "A test package")
                throw std::runtime_error("Expected description 'A test package'");
            if (manifest.sourceDir != "src")
                throw std::runtime_error("Expected source 'src'");
            if (manifest.libraryPath != "build/testpkg.mtcLib")
                throw std::runtime_error("Expected library path");
            if (manifest.dependencies.size() != 1)
                throw std::runtime_error("Expected 1 dependency");
            if (manifest.dependencies.at("dep1") != "^1.0.0")
                throw std::runtime_error("Expected dep1 version '^1.0.0'");
        });

        addCallbackTest("PackageManifest: toJson round-trip", "", [](services::ScriptAPI&) {
            packagemanager::PackageManifest original;
            original.name = "roundtrip";
            original.version = "2.3.4";
            original.sourceDir = "lib";

            std::string json = original.toJson();
            auto parsed = packagemanager::PackageManifest::parseFromJson(json);

            if (parsed.name != "roundtrip" || parsed.version != "2.3.4" || parsed.sourceDir != "lib")
                throw std::runtime_error("Round-trip failed");
        });

        addCallbackTest("PackageManifest: missing name throws", "", [](services::ScriptAPI&) {
            try
            {
                packagemanager::PackageManifest::parseFromJson(R"({"version": "1.0.0"})");
                throw std::runtime_error("Expected exception for missing name");
            }
            catch (const std::runtime_error& e)
            {
                if (std::string(e.what()).find("name") == std::string::npos)
                    throw;
            }
        });

        // =============================================
        // Lockfile Tests
        // =============================================

        addCallbackTest("Lockfile: parse and serialize round-trip", "", [](services::ScriptAPI&) {
            packagemanager::Lockfile original;
            original.version = 1;

            packagemanager::LockedPackage pkg;
            pkg.name = "testpkg";
            pkg.version = "1.2.3";
            pkg.resolved = "file:///tmp/registry/testpkg/1.2.3";
            pkg.integrity = "sha256-abc123";
            pkg.dependencies["dep1"] = "^1.0.0";
            original.packages["testpkg"] = pkg;

            std::string json = original.toJson();
            auto parsed = packagemanager::Lockfile::parseFromJson(json);

            if (parsed.version != 1)
                throw std::runtime_error("Expected version 1");
            if (parsed.packages.size() != 1)
                throw std::runtime_error("Expected 1 package");

            auto& p = parsed.packages.at("testpkg");
            if (p.version != "1.2.3")
                throw std::runtime_error("Expected version '1.2.3'");
            if (p.integrity != "sha256-abc123")
                throw std::runtime_error("Expected integrity 'sha256-abc123'");
            if (p.dependencies.at("dep1") != "^1.0.0")
                throw std::runtime_error("Expected dep1 '^1.0.0'");
        });

        // =============================================
        // Sha256 Tests
        // =============================================

        addCallbackTest("Sha256: empty string hash", "", [](services::ScriptAPI&) {
            std::string hash = packagemanager::Sha256::hash("");
            // Known SHA-256 of empty string
            if (hash != "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
                throw std::runtime_error("SHA-256 of empty string incorrect: " + hash);
        });

        addCallbackTest("Sha256: known string hash", "", [](services::ScriptAPI&) {
            std::string hash = packagemanager::Sha256::hash("hello");
            // Known SHA-256 of "hello"
            if (hash != "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824")
                throw std::runtime_error("SHA-256 of 'hello' incorrect: " + hash);
        });

        addCallbackTest("Sha256: deterministic", "", [](services::ScriptAPI&) {
            std::string h1 = packagemanager::Sha256::hash("test data");
            std::string h2 = packagemanager::Sha256::hash("test data");
            if (h1 != h2)
                throw std::runtime_error("SHA-256 should be deterministic");
        });

        // =============================================
        // PackageRegistry Tests
        // =============================================

        addCallbackTest("PackageRegistry: list available versions", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);

            auto versions = registry.getAvailableVersions("mathlib");
            if (versions.size() != 3)
                throw std::runtime_error("Expected 3 versions of mathlib, got " + std::to_string(versions.size()));

            // Should be sorted
            if (versions[0].toString() != "1.0.0")
                throw std::runtime_error("Expected first version 1.0.0, got " + versions[0].toString());
            if (versions[2].toString() != "2.0.0")
                throw std::runtime_error("Expected last version 2.0.0, got " + versions[2].toString());
        });

        addCallbackTest("PackageRegistry: get manifest", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);

            auto manifest = registry.getManifest("mathlib", "1.2.0");
            if (manifest.name != "mathlib")
                throw std::runtime_error("Expected name 'mathlib'");
            if (manifest.version != "1.2.0")
                throw std::runtime_error("Expected version '1.2.0'");
            if (manifest.dependencies.find("utils") == manifest.dependencies.end())
                throw std::runtime_error("Expected 'utils' dependency");
        });

        addCallbackTest("PackageRegistry: nonexistent package returns empty", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);

            auto versions = registry.getAvailableVersions("nonexistent");
            if (!versions.empty())
                throw std::runtime_error("Expected empty versions for nonexistent package");
        });

        addCallbackTest("PackageRegistry: packageExists", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);

            if (!registry.packageExists("mathlib", "1.0.0"))
                throw std::runtime_error("mathlib@1.0.0 should exist");
            if (registry.packageExists("mathlib", "9.9.9"))
                throw std::runtime_error("mathlib@9.9.9 should not exist");
        });

        // =============================================
        // DependencyResolver Tests
        // =============================================

        addCallbackTest("DependencyResolver: resolve direct dependency", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);
            packagemanager::DependencyResolver resolver(registry);

            std::vector<packagemanager::PackageDependency> deps = {
                {"mathlib", "^1.0.0"}
            };

            auto resolved = resolver.resolve(deps);

            if (resolved.find("mathlib") == resolved.end())
                throw std::runtime_error("Expected mathlib to be resolved");

            // ^1.0.0 should pick 1.2.0 (highest in 1.x, since 2.0.0 is outside ^1)
            if (resolved["mathlib"].version != "1.2.0")
                throw std::runtime_error("Expected mathlib 1.2.0, got " + resolved["mathlib"].version);
        });

        addCallbackTest("DependencyResolver: resolve transitive dependency", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);
            packagemanager::DependencyResolver resolver(registry);

            // mathlib@1.2.0 depends on utils@^2.0.0
            std::vector<packagemanager::PackageDependency> deps = {
                {"mathlib", "^1.2.0"}
            };

            auto resolved = resolver.resolve(deps);

            if (resolved.find("mathlib") == resolved.end())
                throw std::runtime_error("Expected mathlib to be resolved");
            if (resolved.find("utils") == resolved.end())
                throw std::runtime_error("Expected transitive dep 'utils' to be resolved");
            if (resolved["utils"].version != "2.1.0")
                throw std::runtime_error("Expected utils 2.1.0, got " + resolved["utils"].version);
        });

        addCallbackTest("DependencyResolver: exact version constraint", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);
            packagemanager::DependencyResolver resolver(registry);

            std::vector<packagemanager::PackageDependency> deps = {
                {"mathlib", "1.0.0"}
            };

            auto resolved = resolver.resolve(deps);
            if (resolved["mathlib"].version != "1.0.0")
                throw std::runtime_error("Expected exact mathlib 1.0.0, got " + resolved["mathlib"].version);
        });

        addCallbackTest("DependencyResolver: circular dependency detected", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);
            packagemanager::DependencyResolver resolver(registry);

            std::vector<packagemanager::PackageDependency> deps = {
                {"circular-a", "1.0.0"}
            };

            try
            {
                resolver.resolve(deps);
                throw std::runtime_error("Expected circular dependency error");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("Circular") == std::string::npos && msg.find("circular") == std::string::npos)
                    throw std::runtime_error("Expected 'circular' in error, got: " + msg);
            }
        });

        addCallbackTest("DependencyResolver: nonexistent package throws", "", [](services::ScriptAPI&) {
            std::string registryPath = "mType/tests/testFiles/packagemanager/registry";
            packagemanager::PackageRegistry registry(registryPath);
            packagemanager::DependencyResolver resolver(registry);

            std::vector<packagemanager::PackageDependency> deps = {
                {"nonexistent", "^1.0.0"}
            };

            try
            {
                resolver.resolve(deps);
                throw std::runtime_error("Expected error for nonexistent package");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("not found") == std::string::npos && msg.find("Not found") == std::string::npos)
                    throw std::runtime_error("Expected 'not found' in error, got: " + msg);
            }
        });

        // =============================================
        // PackageInstaller Tests
        // =============================================

        addCallbackTest("PackageInstaller: install creates mt_modules", "", [](services::ScriptAPI&) {
            // Use a temp directory for the test project
            fs::path tempDir = fs::temp_directory_path() / "_mtype_pkg_test_install";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            fs::create_directories(tempDir);

            std::string registryPath = fs::canonical("mType/tests/testFiles/packagemanager/registry").string();

            packagemanager::PackageInstaller installer(tempDir.string(), registryPath);

            std::vector<packagemanager::PackageDependency> deps = {
                {"mathlib", "1.0.0"}
            };

            auto result = installer.install(deps);

            if (!result.success)
            {
                std::string errors;
                for (const auto& e : result.errors) errors += e + "; ";
                fs::remove_all(tempDir);
                throw std::runtime_error("Install failed: " + errors);
            }

            if (result.installed != 1)
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("Expected 1 installed, got " + std::to_string(result.installed));
            }

            // Verify mt_modules/@mathlib/ exists
            if (!fs::exists(tempDir / "mt_modules" / "@mathlib" / "mtpkg.json"))
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("mt_modules/@mathlib/mtpkg.json not found");
            }

            // Verify lockfile exists
            if (!fs::exists(tempDir / "mtproj.lock"))
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("mtproj.lock not found");
            }

            fs::remove_all(tempDir);
        });

        addCallbackTest("PackageInstaller: install with transitive deps", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_pkg_test_transitive";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            fs::create_directories(tempDir);

            std::string registryPath = fs::canonical("mType/tests/testFiles/packagemanager/registry").string();

            packagemanager::PackageInstaller installer(tempDir.string(), registryPath);

            // mathlib@1.2.0 depends on utils@^2.0.0
            std::vector<packagemanager::PackageDependency> deps = {
                {"mathlib", "^1.2.0"}
            };

            auto result = installer.install(deps);

            if (!result.success)
            {
                std::string errors;
                for (const auto& e : result.errors) errors += e + "; ";
                fs::remove_all(tempDir);
                throw std::runtime_error("Install failed: " + errors);
            }

            if (result.installed != 2)
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("Expected 2 installed (mathlib + utils), got " + std::to_string(result.installed));
            }

            if (!fs::exists(tempDir / "mt_modules" / "@mathlib"))
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("@mathlib not installed");
            }
            if (!fs::exists(tempDir / "mt_modules" / "@utils"))
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("@utils not installed (transitive dep)");
            }

            fs::remove_all(tempDir);
        });

        // =============================================
        // MtModulesManager Tests
        // =============================================

        addCallbackTest("MtModulesManager: getAliases from installed packages", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_pkg_test_aliases";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            fs::create_directories(tempDir);

            std::string registryPath = fs::canonical("mType/tests/testFiles/packagemanager/registry").string();

            // Install a package first
            packagemanager::PackageInstaller installer(tempDir.string(), registryPath);
            std::vector<packagemanager::PackageDependency> deps = {{"mathlib", "1.0.0"}};
            installer.install(deps);

            // Now test MtModulesManager
            packagemanager::MtModulesManager mgr(tempDir.string());
            auto aliases = mgr.getAliases();

            if (aliases.find("@mathlib") == aliases.end())
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("Expected '@mathlib' alias");
            }

            if (aliases["@mathlib"].empty())
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("'@mathlib' alias has empty path");
            }

            auto installed = mgr.getInstalledPackages();
            if (installed.size() != 1 || installed[0] != "mathlib")
            {
                fs::remove_all(tempDir);
                throw std::runtime_error("Expected ['mathlib'] installed");
            }

            fs::remove_all(tempDir);
        });

        // =============================================
        // ProjectConfig Dependencies Parsing
        // =============================================

        addCallbackTest("ProjectConfig: parse Dependencies section", "", [](services::ScriptAPI&) {
            project::ProjectConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/packagemanager/project/TestPkgProject.mtproj");

            if (config->dependencies.packages.size() != 1)
                throw std::runtime_error("Expected 1 dependency, got " +
                    std::to_string(config->dependencies.packages.size()));

            auto& dep = config->dependencies.packages[0];
            if (dep.name != "mathlib")
                throw std::runtime_error("Expected dep name 'mathlib', got '" + dep.name + "'");
            if (dep.versionRange != "^1.0.0")
                throw std::runtime_error("Expected dep version '^1.0.0', got '" + dep.versionRange + "'");
        });

        addCallbackTest("ProjectConfig: empty Dependencies section parses OK", "", [](services::ScriptAPI&) {
            // The existing TestProject.mtproj has no Dependencies section
            project::ProjectConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/projectManifest/TestProject.mtproj");

            if (!config->dependencies.packages.empty())
                throw std::runtime_error("Expected 0 dependencies for project without Dependencies section");
        });

        // MYT-323: source globs must not sweep in installed-dep source from
        // mt_modules/. A `<Include>**/*.mt</Include>` pattern means "my
        // source", not "my source plus everything mtpm has put on disk".
        addCallbackTest("ProjectConfig: **/*.mt glob excludes mt_modules/ contents", "", [](services::ScriptAPI&) {
            fs::path projDir = fs::temp_directory_path() / "_mtype_pcparser_test_mtmod";
            if (fs::exists(projDir)) fs::remove_all(projDir);
            fs::create_directories(projDir / "src");
            fs::create_directories(projDir / "mt_modules" / "@somelib");

            // Project source
            { std::ofstream f(projDir / "src" / "Main.mt"); f << "// project source\n"; }
            // Dep package source — must NOT appear in resolvedSourceFiles
            { std::ofstream f(projDir / "mt_modules" / "@somelib" / "Foo.mt"); f << "// dep source\n"; }

            // Minimal .mtproj with broad glob
            { std::ofstream f(projDir / "test.mtproj");
              f << "<Project Name=\"t\" Version=\"1.0.0\">\n"
                << "  <Source><Include>**/*.mt</Include></Source>\n"
                << "  <Output Directory=\"build\" />\n"
                << "</Project>\n"; }

            project::ProjectConfigParser parser;
            auto config = parser.parse((projDir / "test.mtproj").string());

            auto cleanup = [&]() { fs::remove_all(projDir); };

            bool foundProjectSrc = false;
            bool foundDepSrc = false;
            for (const auto& f : config->resolvedSourceFiles)
            {
                if (f.find("Main.mt") != std::string::npos) foundProjectSrc = true;
                if (f.find("Foo.mt") != std::string::npos) foundDepSrc = true;
            }

            if (!foundProjectSrc)
            {
                cleanup();
                throw std::runtime_error("src/Main.mt should be in resolvedSourceFiles");
            }
            if (foundDepSrc)
            {
                cleanup();
                throw std::runtime_error(
                    "mt_modules/@somelib/Foo.mt must NOT be in resolvedSourceFiles "
                    "(it should be alias-resolved at import time, not pre-bundled)");
            }
            cleanup();
        });

        addCallbackTest("ProjectConfig: source glob also excludes the output dir", "", [](services::ScriptAPI&) {
            // A user re-running --build shouldn't have prior-build .mt files
            // (if any test fixture stages them there) swept back in.
            fs::path projDir = fs::temp_directory_path() / "_mtype_pcparser_test_outdir";
            if (fs::exists(projDir)) fs::remove_all(projDir);
            fs::create_directories(projDir / "src");
            fs::create_directories(projDir / "build" / "stale");

            { std::ofstream f(projDir / "src" / "Main.mt"); f << "\n"; }
            { std::ofstream f(projDir / "build" / "stale" / "Old.mt"); f << "\n"; }

            { std::ofstream f(projDir / "test.mtproj");
              f << "<Project Name=\"t\" Version=\"1.0.0\">\n"
                << "  <Source><Include>**/*.mt</Include></Source>\n"
                << "  <Output Directory=\"build\" />\n"
                << "</Project>\n"; }

            project::ProjectConfigParser parser;
            auto config = parser.parse((projDir / "test.mtproj").string());

            auto cleanup = [&]() { fs::remove_all(projDir); };

            for (const auto& f : config->resolvedSourceFiles)
            {
                if (f.find("Old.mt") != std::string::npos)
                {
                    cleanup();
                    throw std::runtime_error(
                        "Output-directory .mt files must not be in resolvedSourceFiles");
                }
            }
            cleanup();
        });

        // =============================================
        // Publish Tests
        // =============================================

        addCallbackTest("Publish: round-trip stages package and prints integrity", "", [](services::ScriptAPI&) {
            fs::path tempProject = fs::temp_directory_path() / "_mtype_pkg_test_publish_rt";
            fs::path tempRegistry = fs::temp_directory_path() / "_mtype_pkg_test_publish_rt_reg";
            if (fs::exists(tempProject)) fs::remove_all(tempProject);
            if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
            fs::create_directories(tempProject / "src");
            fs::create_directories(tempRegistry);

            { std::ofstream f(tempProject / "mtpkg.json");
              f << "{\"name\":\"pubtest\",\"version\":\"1.0.0\"}"; }
            { std::ofstream f(tempProject / "src" / "Main.mt");
              f << "// hello\n"; }

            packagemanager::PublishOptions opts;
            opts.projectDir = tempProject.string();
            auto result = packagemanager::publish(tempRegistry.string(), opts, nullptr);

            auto cleanup = [&] {
                fs::remove_all(tempProject);
                fs::remove_all(tempRegistry);
            };

            if (!result.success)
            {
                std::string errors;
                for (const auto& e : result.errors) errors += e + "; ";
                cleanup();
                throw std::runtime_error("Publish failed: " + errors);
            }

            fs::path expected = tempRegistry / "pubtest" / "1.0.0";
            if (!fs::exists(expected / "mtpkg.json"))
            {
                cleanup();
                throw std::runtime_error("Expected mtpkg.json at registry destination");
            }
            if (!fs::exists(expected / "src" / "Main.mt"))
            {
                cleanup();
                throw std::runtime_error("Expected src/Main.mt at registry destination");
            }
            if (result.integrity.rfind("sha256-", 0) != 0)
            {
                cleanup();
                throw std::runtime_error("Integrity hash missing sha256- prefix: " + result.integrity);
            }
            std::string recomputed = "sha256-" + packagemanager::Sha256::hashDirectory(expected.string());
            if (recomputed != result.integrity)
            {
                cleanup();
                throw std::runtime_error("Reported integrity does not match recomputed hash");
            }

            cleanup();
        });

        addCallbackTest("Publish: skips .git, mt_modules, mtproj.lock", "", [](services::ScriptAPI&) {
            fs::path tempProject = fs::temp_directory_path() / "_mtype_pkg_test_publish_skip";
            fs::path tempRegistry = fs::temp_directory_path() / "_mtype_pkg_test_publish_skip_reg";
            if (fs::exists(tempProject)) fs::remove_all(tempProject);
            if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
            fs::create_directories(tempProject / "src");
            fs::create_directories(tempProject / ".git");
            fs::create_directories(tempProject / "mt_modules" / "@x");
            fs::create_directories(tempRegistry);

            { std::ofstream f(tempProject / "mtpkg.json");
              f << "{\"name\":\"skiptest\",\"version\":\"1.0.0\"}"; }
            { std::ofstream f(tempProject / "src" / "Main.mt"); f << "x\n"; }
            { std::ofstream f(tempProject / ".git" / "HEAD"); f << "ref:foo\n"; }
            { std::ofstream f(tempProject / "mt_modules" / "@x" / "x.mt"); f << "y\n"; }
            { std::ofstream f(tempProject / "mtproj.lock"); f << "{}\n"; }

            packagemanager::PublishOptions opts;
            opts.projectDir = tempProject.string();
            auto result = packagemanager::publish(tempRegistry.string(), opts, nullptr);

            auto cleanup = [&] {
                fs::remove_all(tempProject);
                fs::remove_all(tempRegistry);
            };

            if (!result.success)
            {
                cleanup();
                throw std::runtime_error("Publish failed unexpectedly");
            }

            fs::path dest = tempRegistry / "skiptest" / "1.0.0";
            if (fs::exists(dest / ".git"))
            {
                cleanup();
                throw std::runtime_error(".git directory was not skipped");
            }
            if (fs::exists(dest / "mt_modules"))
            {
                cleanup();
                throw std::runtime_error("mt_modules directory was not skipped");
            }
            if (fs::exists(dest / "mtproj.lock"))
            {
                cleanup();
                throw std::runtime_error("mtproj.lock was not skipped");
            }
            if (!fs::exists(dest / "src" / "Main.mt"))
            {
                cleanup();
                throw std::runtime_error("src/Main.mt should still be present");
            }

            cleanup();
        });

        addCallbackTest("Publish: refuses to overwrite without --force", "", [](services::ScriptAPI&) {
            fs::path tempProject = fs::temp_directory_path() / "_mtype_pkg_test_publish_overwrite";
            fs::path tempRegistry = fs::temp_directory_path() / "_mtype_pkg_test_publish_overwrite_reg";
            if (fs::exists(tempProject)) fs::remove_all(tempProject);
            if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
            fs::create_directories(tempProject);
            fs::create_directories(tempRegistry);

            { std::ofstream f(tempProject / "mtpkg.json");
              f << "{\"name\":\"owtest\",\"version\":\"1.0.0\"}"; }

            packagemanager::PublishOptions opts;
            opts.projectDir = tempProject.string();
            auto first = packagemanager::publish(tempRegistry.string(), opts, nullptr);

            auto cleanup = [&] {
                fs::remove_all(tempProject);
                fs::remove_all(tempRegistry);
            };

            if (!first.success)
            {
                cleanup();
                throw std::runtime_error("First publish should have succeeded");
            }

            auto second = packagemanager::publish(tempRegistry.string(), opts, nullptr);
            if (second.success)
            {
                cleanup();
                throw std::runtime_error("Second publish should have failed without --force");
            }
            bool mentionsForce = false;
            for (const auto& e : second.errors)
            {
                if (e.find("--force") != std::string::npos) { mentionsForce = true; break; }
            }
            if (!mentionsForce)
            {
                cleanup();
                throw std::runtime_error("Error message should mention --force");
            }

            opts.force = true;
            auto third = packagemanager::publish(tempRegistry.string(), opts, nullptr);
            if (!third.success)
            {
                cleanup();
                throw std::runtime_error("Publish with --force should have succeeded");
            }

            cleanup();
        });

        // =============================================
        // Install Integrity Verification Tests
        // =============================================

        addCallbackTest("Install: integrity mismatch fails", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_pkg_test_integrity_fail";
            fs::path tempRegistry = fs::temp_directory_path() / "_mtype_pkg_test_integrity_fail_reg";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
            fs::create_directories(tempDir);
            fs::create_directories(tempRegistry);

            // Clone the shared mathlib@1.0.0 into our temp registry so we can mutate it.
            fs::path sharedRegistry = fs::canonical("mType/tests/testFiles/packagemanager/registry");
            fs::create_directories(tempRegistry / "mathlib" / "1.0.0");
            fs::copy(sharedRegistry / "mathlib" / "1.0.0",
                     tempRegistry / "mathlib" / "1.0.0",
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);

            auto cleanup = [&] {
                fs::remove_all(tempDir);
                fs::remove_all(tempRegistry);
            };

            // First install — produces lockfile with current integrity hash
            packagemanager::PackageInstaller installer(tempDir.string(), tempRegistry.string());
            std::vector<packagemanager::PackageDependency> deps = {{"mathlib", "1.0.0"}};
            auto firstResult = installer.install(deps);
            if (!firstResult.success)
            {
                std::string errors;
                for (const auto& e : firstResult.errors) errors += e + "; ";
                cleanup();
                throw std::runtime_error("Initial install failed: " + errors);
            }

            // Tamper with the registry copy. We avoid touching mtpkg.json so
            // that a subsequent cold resolution can still parse it.
            fs::path victim = tempRegistry / "mathlib" / "1.0.0" / "src" / "Vector.mt";
            { std::ofstream f(victim, std::ios::app); f << "\n// tampered\n"; }

            // Second install — must fail with integrity mismatch
            packagemanager::PackageInstaller installer2(tempDir.string(), tempRegistry.string());
            auto secondResult = installer2.install(deps);
            if (secondResult.success)
            {
                cleanup();
                throw std::runtime_error("Second install should have failed (integrity mismatch)");
            }
            bool mentionsMismatch = false;
            bool mentionsMathlib = false;
            for (const auto& e : secondResult.errors)
            {
                if (e.find("Integrity mismatch") != std::string::npos) mentionsMismatch = true;
                if (e.find("mathlib") != std::string::npos) mentionsMathlib = true;
            }
            if (!mentionsMismatch || !mentionsMathlib)
            {
                cleanup();
                throw std::runtime_error("Error should name 'Integrity mismatch' and 'mathlib'");
            }

            cleanup();
        });

        addCallbackTest("Install: --force-resolve bypasses mismatch and rewrites lockfile", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_pkg_test_force_resolve";
            fs::path tempRegistry = fs::temp_directory_path() / "_mtype_pkg_test_force_resolve_reg";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            if (fs::exists(tempRegistry)) fs::remove_all(tempRegistry);
            fs::create_directories(tempDir);
            fs::create_directories(tempRegistry);

            fs::path sharedRegistry = fs::canonical("mType/tests/testFiles/packagemanager/registry");
            fs::create_directories(tempRegistry / "mathlib" / "1.0.0");
            fs::copy(sharedRegistry / "mathlib" / "1.0.0",
                     tempRegistry / "mathlib" / "1.0.0",
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);

            auto cleanup = [&] {
                fs::remove_all(tempDir);
                fs::remove_all(tempRegistry);
            };

            packagemanager::PackageInstaller installer(tempDir.string(), tempRegistry.string());
            std::vector<packagemanager::PackageDependency> deps = {{"mathlib", "1.0.0"}};
            auto firstResult = installer.install(deps);
            if (!firstResult.success)
            {
                cleanup();
                throw std::runtime_error("Initial install failed");
            }

            // Tamper with the registry copy. We avoid touching mtpkg.json so
            // that a subsequent cold resolution can still parse it.
            fs::path victim = tempRegistry / "mathlib" / "1.0.0" / "src" / "Vector.mt";
            { std::ofstream f(victim, std::ios::app); f << "\n// tampered\n"; }

            std::string newHash = "sha256-" +
                packagemanager::Sha256::hashDirectory((tempRegistry / "mathlib" / "1.0.0").string());

            // Re-install with --force-resolve — should succeed and rewrite the lockfile
            packagemanager::PackageInstaller installer2(tempDir.string(), tempRegistry.string());
            installer2.setForceResolve(true);
            auto secondResult = installer2.install(deps);
            if (!secondResult.success)
            {
                std::string errors;
                for (const auto& e : secondResult.errors) errors += e + "; ";
                cleanup();
                throw std::runtime_error("--force-resolve install failed: " + errors);
            }

            auto lockfile = packagemanager::Lockfile::loadFromFile((tempDir / "mtproj.lock").string());
            auto it = lockfile.packages.find("mathlib");
            if (it == lockfile.packages.end())
            {
                cleanup();
                throw std::runtime_error("Lockfile missing mathlib entry after --force-resolve");
            }
            if (it->second.integrity != newHash)
            {
                cleanup();
                throw std::runtime_error(
                    "Lockfile integrity not rewritten: expected " + newHash +
                    ", got " + it->second.integrity);
            }

            cleanup();
        });

        // =============================================
        // PackageCompiler Tests (MYT-323)
        // =============================================
        // The compiler bridges the source/bytecode gap: mtpm install resolves
        // source into <registry>/<name>/<ver>/, the compiler turns that into a
        // <name>.mtcLib in the same dir. These tests cover the pure helpers
        // (topo order + synthetic .mtproj) so the subprocess wiring can be
        // verified end-to-end at integration time.

        addCallbackTest("PackageCompiler: topologicalOrder produces deps before dependents", "", [](services::ScriptAPI&) {
            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;

            packagemanager::ResolvedPackage stdlib;
            stdlib.name = "stdlib";
            stdlib.version = "1.0.0";
            resolved["stdlib"] = stdlib;

            packagemanager::ResolvedPackage app;
            app.name = "app";
            app.version = "1.0.0";
            app.dependencies["stdlib"] = "1.0.0";
            resolved["app"] = app;

            auto order = packagemanager::PackageCompiler::topologicalOrder(resolved);
            if (order.size() != 2)
                throw std::runtime_error("Expected 2 nodes, got " + std::to_string(order.size()));

            auto stdlibIdx = std::find(order.begin(), order.end(), "stdlib") - order.begin();
            auto appIdx = std::find(order.begin(), order.end(), "app") - order.begin();
            if (stdlibIdx >= appIdx)
                throw std::runtime_error("Expected stdlib before app in topological order");
        });

        addCallbackTest("PackageCompiler: topologicalOrder handles diamond dependency", "", [](services::ScriptAPI&) {
            // base ← left, right ← top
            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            for (const auto& n : {"base", "left", "right", "top"})
            {
                packagemanager::ResolvedPackage p;
                p.name = n;
                p.version = "1.0.0";
                resolved[n] = p;
            }
            resolved["left"].dependencies["base"] = "1.0.0";
            resolved["right"].dependencies["base"] = "1.0.0";
            resolved["top"].dependencies["left"] = "1.0.0";
            resolved["top"].dependencies["right"] = "1.0.0";

            auto order = packagemanager::PackageCompiler::topologicalOrder(resolved);
            if (order.size() != 4)
                throw std::runtime_error("Expected 4 nodes");

            auto pos = [&](const std::string& n) {
                return std::find(order.begin(), order.end(), n) - order.begin();
            };
            if (pos("base") >= pos("left")) throw std::runtime_error("base must come before left");
            if (pos("base") >= pos("right")) throw std::runtime_error("base must come before right");
            if (pos("left") >= pos("top")) throw std::runtime_error("left must come before top");
            if (pos("right") >= pos("top")) throw std::runtime_error("right must come before top");
        });

        addCallbackTest("PackageCompiler: topologicalOrder ignores external deps", "", [](services::ScriptAPI&) {
            // Only `pkg` is in `resolved`; its declared "external" dep is not.
            // The sort must not crash or include the missing edge target.
            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            packagemanager::ResolvedPackage pkg;
            pkg.name = "pkg";
            pkg.version = "1.0.0";
            pkg.dependencies["external"] = "1.0.0";
            resolved["pkg"] = pkg;

            auto order = packagemanager::PackageCompiler::topologicalOrder(resolved);
            if (order.size() != 1 || order[0] != "pkg")
                throw std::runtime_error("Expected order=[pkg], got size " + std::to_string(order.size()));
        });

        addCallbackTest("PackageCompiler: buildSyntheticMtproj emits valid XML", "", [](services::ScriptAPI&) {
            packagemanager::ResolvedPackage pkg;
            pkg.name = "mathlib";
            pkg.version = "1.0.0";

            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            resolved["mathlib"] = pkg;

            std::string xml = packagemanager::PackageCompiler::buildSyntheticMtproj(
                pkg, "src/**/*.mt", resolved);

            if (xml.find("<Project Name=\"mathlib\"") == std::string::npos)
                throw std::runtime_error("Missing Project tag with name");
            if (xml.find("Version=\"1.0.0\"") == std::string::npos)
                throw std::runtime_error("Missing Version attribute");
            if (xml.find("<Include>src/**/*.mt</Include>") == std::string::npos)
                throw std::runtime_error("Missing Include glob");
            if (xml.find("<Output Directory=\".\"") == std::string::npos)
                throw std::runtime_error("Missing Output Directory");
            // No dependencies declared — no <Dependencies> section
            if (xml.find("<Dependencies>") != std::string::npos)
                throw std::runtime_error("Should not emit <Dependencies> when none declared");
        });

        addCallbackTest("PackageCompiler: buildSyntheticMtproj pins to exact resolved versions", "", [](services::ScriptAPI&) {
            // mtype-sfml declares ^0.0.1 of mTypeLib but resolved to 0.0.1.
            // Synthesized .mtproj must emit "0.0.1" (exact) so MtcPathResolver
            // finds <registry>/mTypeLib/0.0.1/mTypeLib.mtcLib without semver math.
            packagemanager::ResolvedPackage stdlib;
            stdlib.name = "mTypeLib";
            stdlib.version = "0.0.1";

            packagemanager::ResolvedPackage app;
            app.name = "mtype-sfml";
            app.version = "0.0.1";
            app.dependencies["mTypeLib"] = "^0.0.1";  // declared range

            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            resolved["mTypeLib"] = stdlib;
            resolved["mtype-sfml"] = app;

            std::string xml = packagemanager::PackageCompiler::buildSyntheticMtproj(
                app, "**/*.mt", resolved);

            if (xml.find("<Dependencies>") == std::string::npos)
                throw std::runtime_error("Should emit <Dependencies>");
            if (xml.find("<Package Name=\"mTypeLib\" Version=\"0.0.1\"") == std::string::npos)
                throw std::runtime_error("Should pin to exact 0.0.1, got: " + xml);
            // The original range must NOT appear
            if (xml.find("Version=\"^0.0.1\"") != std::string::npos)
                throw std::runtime_error("Range '^0.0.1' leaked into synthetic .mtproj");
        });

        addCallbackTest("PackageCompiler: buildSyntheticMtproj escapes XML metacharacters", "", [](services::ScriptAPI&) {
            // Names should never contain these but a malicious or buggy
            // mtpkg.json could. Confirm escape happens defensively.
            packagemanager::ResolvedPackage pkg;
            pkg.name = "a&b";
            pkg.version = "1<2";

            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            resolved["a&b"] = pkg;

            std::string xml = packagemanager::PackageCompiler::buildSyntheticMtproj(
                pkg, "**/*.mt", resolved);

            if (xml.find("a&amp;b") == std::string::npos)
                throw std::runtime_error("Name '&' not escaped");
            if (xml.find("1&lt;2") == std::string::npos)
                throw std::runtime_error("Version '<' not escaped");
        });

        addCallbackTest("PackageCompiler: discoverMTypeExecutable handles sibling install", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_compiler_test_sibling";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            fs::create_directories(tempDir);

            fs::path mtpmExe = tempDir / "mtpm.exe";
#ifdef _WIN32
            fs::path mTypeExe = tempDir / "mType.exe";
#else
            fs::path mTypeExe = tempDir / "mType";
#endif
            { std::ofstream f(mtpmExe); f << "binary"; }
            { std::ofstream f(mTypeExe); f << "binary"; }

            std::string found = packagemanager::PackageCompiler::discoverMTypeExecutable(mtpmExe.string());

            auto cleanup = [&]() { fs::remove_all(tempDir); };

            if (found.empty())
            {
                cleanup();
                throw std::runtime_error("Expected sibling discovery to succeed");
            }
            if (fs::canonical(found) != fs::canonical(mTypeExe))
            {
                cleanup();
                throw std::runtime_error("Expected mType at " + mTypeExe.string() + ", got " + found);
            }

            cleanup();
        });

        addCallbackTest("PackageCompiler: discoverMTypeExecutable handles dev bin layout", "", [](services::ScriptAPI&) {
            // bin/mtpm/Release/x64/mtpm.exe ↔ bin/mType/Release/x64/mType.exe
            fs::path tempBin = fs::temp_directory_path() / "_mtype_compiler_test_devlayout";
            if (fs::exists(tempBin)) fs::remove_all(tempBin);
            fs::create_directories(tempBin / "mtpm" / "Release" / "x64");
            fs::create_directories(tempBin / "mType" / "Release" / "x64");

            fs::path mtpmExe = tempBin / "mtpm" / "Release" / "x64" / "mtpm.exe";
#ifdef _WIN32
            fs::path mTypeExe = tempBin / "mType" / "Release" / "x64" / "mType.exe";
#else
            fs::path mTypeExe = tempBin / "mType" / "Release" / "x64" / "mType";
#endif
            { std::ofstream f(mtpmExe); f << "binary"; }
            { std::ofstream f(mTypeExe); f << "binary"; }

            std::string found = packagemanager::PackageCompiler::discoverMTypeExecutable(mtpmExe.string());

            auto cleanup = [&]() { fs::remove_all(tempBin); };

            if (found.empty())
            {
                cleanup();
                throw std::runtime_error("Expected dev-layout discovery to succeed");
            }
            if (fs::canonical(found) != fs::canonical(mTypeExe))
            {
                cleanup();
                throw std::runtime_error("Expected mType at " + mTypeExe.string() + ", got " + found);
            }

            cleanup();
        });

        addCallbackTest("PackageCompiler: discoverMTypeExecutable returns empty when nothing found", "", [](services::ScriptAPI&) {
            fs::path tempDir = fs::temp_directory_path() / "_mtype_compiler_test_missing";
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            fs::create_directories(tempDir);

            fs::path mtpmExe = tempDir / "mtpm.exe";
            { std::ofstream f(mtpmExe); f << "binary"; }

            std::string found = packagemanager::PackageCompiler::discoverMTypeExecutable(mtpmExe.string());
            fs::remove_all(tempDir);

            if (!found.empty())
                throw std::runtime_error("Expected empty result when no mType exists, got: " + found);
        });

        addCallbackTest("PackageCompiler: compileAll without executable surfaces clear error", "", [](services::ScriptAPI&) {
            // Sanity check: invoking compileAll with no executable set must not
            // hang trying to subprocess into a missing binary — should fail fast
            // with a descriptive error.
            packagemanager::ResolvedPackage pkg;
            pkg.name = "x";
            pkg.version = "1.0.0";
            pkg.registryPath = "/nonexistent";

            std::unordered_map<std::string, packagemanager::ResolvedPackage> resolved;
            resolved["x"] = pkg;

            packagemanager::PackageCompiler compiler;
            auto result = compiler.compileAll(resolved);

            if (result.success)
                throw std::runtime_error("Expected failure when mType.exe path is unset");
            if (result.errors.empty())
                throw std::runtime_error("Expected an error message");
        });
    }
}
