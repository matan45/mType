#include "PackageManagerTestSuite.hpp"
#include "SemVer.hpp"
#include "PackageManifest.hpp"
#include "Lockfile.hpp"
#include "PackageRegistry.hpp"
#include "DependencyResolver.hpp"
#include "PackageInstaller.hpp"
#include "MtModulesManager.hpp"
#include "Sha256.hpp"
#include "../../project/ProjectConfigParser.hpp"
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
    }
}
