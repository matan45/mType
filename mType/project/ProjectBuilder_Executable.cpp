#include "ProjectBuilder.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <algorithm>
#include <cctype>
#include <system_error>
#include "mtclib/LibraryLinker.hpp"
#include "MtModulesManager.hpp"

namespace project
{
    BuildResult ProjectBuilder::buildExecutable(const ProjectConfig& config,
                                                const std::string& outputPath,
                                                const std::string& launcherPath)
    {
        BuildResult result;
        auto startTime = std::chrono::steady_clock::now();

        const auto& sourceFiles = config.resolvedSourceFiles;

        if (sourceFiles.empty())
        {
            result.errors.push_back("No source files to compile");
            result.success = false;
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }

        if (!std::filesystem::exists(launcherPath))
        {
            result.errors.push_back("Launcher binary not found: " + launcherPath);
            result.success = false;
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }

        try
        {
            reportProgress(1, 2, "Compiling bytecode...");

            auto program = compileToProgram(config);

            std::ostringstream bytecodeStream;
            program.serialize(bytecodeStream);
            std::string bytecodeBlob = bytecodeStream.str();

            reportProgress(2, 2, "Embedding bytecode...");

            std::filesystem::path outPath(outputPath);
            if (outPath.has_parent_path() && !std::filesystem::exists(outPath.parent_path()))
            {
                std::filesystem::create_directories(outPath.parent_path());
            }

            std::filesystem::copy_file(launcherPath, outputPath,
                                       std::filesystem::copy_options::overwrite_existing);

            // Append bytecode blob + 8-byte size footer + magic.
            std::ofstream outFile(outputPath, std::ios::binary | std::ios::app);
            if (!outFile)
            {
                throw std::runtime_error("Could not open output file for appending: " + outputPath);
            }

            outFile.write(bytecodeBlob.data(), static_cast<std::streamsize>(bytecodeBlob.size()));

            // Little-endian native (see static_assert in Launcher.cpp).
            uint64_t blobSize = bytecodeBlob.size();
            outFile.write(reinterpret_cast<const char*>(&blobSize), sizeof(blobSize));

            const char magic[4] = { 'M', 'T', 'E', 'X' };
            outFile.write(magic, 4);

            outFile.close();

            if (!config.dependencies.packages.empty())
            {
                std::filesystem::path libsDir = outPath.parent_path() / "libs";
                std::filesystem::create_directories(libsDir);

                mtclib::LibraryLinker linker(config.projectRoot);
                for (const auto& sp : config.imports.searchPaths) {
                    auto absPath = std::filesystem::path(config.projectRoot) / sp;
                    linker.addSearchPath(absPath.string());
                }

                namespace fs = std::filesystem;
                fs::path projectRootCanonical;
                {
                    std::error_code ec;
                    projectRootCanonical = fs::weakly_canonical(fs::path(config.projectRoot), ec);
                    if (ec) {
                        // On symlinked project paths a non-canonical fallback
                        // can make fs::relative produce wrong relative paths
                        // (e.g. "../foo" instead of "foo"), which would mirror
                        // DLLs outside the build dir. Warn so this is debuggable
                        // instead of producing a quietly broken exe.
                        std::cerr << "Warning: failed to canonicalize project root '"
                                  << config.projectRoot << "' (" << ec.message()
                                  << "). Falling back to the raw path — native "
                                  << "plugin binaries may be mirrored to the wrong "
                                  << "directory on systems with symlinks.\n";
                        projectRootCanonical = fs::path(config.projectRoot);
                    }
                }
                fs::path buildRoot = outPath.parent_path();

                // Mirror native plugin binaries from a directory tree into the
                // build output, preserving each binary's path relative to the
                // project root. Used for both the .mtcLib sibling-dir case
                // (binary-distributed dep) and the mt_modules package-root
                // case (source-distributed dep). Without this, user code that
                // calls __plugin_load("mt_modules/.../foo.dll") fails at
                // runtime once the exe is moved out of the project (CWD =
                // build/), because PluginLoader can't find the DLL.
                //
                // Recursive so packages with native binaries under subdirs
                // (e.g. mt_modules/@mtype-sfml/mt/*.dll) are picked up.
                // Skipped entirely for trees outside the project root —
                // registry-installed packages (e.g. ~/.mtype/...) have no
                // project-relative path to preserve and need a different
                // scheme, out of scope here.
                static const std::array<std::string_view, 3> kNativeExtensions = {
                    ".dll", ".so", ".dylib"
                };
                auto mirrorNativeBinaries = [&](const fs::path& packageDir) {
                    std::error_code ec;
                    fs::path packageCanonical = fs::weakly_canonical(packageDir, ec);
                    if (ec) return;
                    auto relFromProject = fs::relative(packageCanonical, projectRootCanonical, ec);
                    if (ec || relFromProject.empty() ||
                        relFromProject.generic_string().rfind("..", 0) == 0) {
                        return;
                    }
                    fs::recursive_directory_iterator walkIt(packageCanonical,
                        fs::directory_options::skip_permission_denied, ec);
                    if (ec) return;
                    for (auto it = walkIt; it != fs::recursive_directory_iterator(); it.increment(ec)) {
                        if (ec) { ec.clear(); continue; }
                        // Skip .git noise — fast-forwards through large repos.
                        if (it->is_directory(ec) &&
                            it->path().filename() == ".git") {
                            it.disable_recursion_pending();
                            continue;
                        }
                        if (!it->is_regular_file(ec)) continue;
                        std::string ext = it->path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(),
                            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        bool isNative = false;
                        for (const auto& nx : kNativeExtensions) {
                            if (ext == nx) { isNative = true; break; }
                        }
                        if (!isNative) continue;

                        // Recompute the file's path relative to projectRoot so
                        // both `mt_modules/@pkg/mt/foo.dll` and the package
                        // root case land at their correct mirror locations.
                        auto fileRel = fs::relative(it->path(), projectRootCanonical, ec);
                        if (ec || fileRel.empty() ||
                            fileRel.generic_string().rfind("..", 0) == 0) {
                            continue;
                        }
                        fs::path destNative = buildRoot / fileRel;
                        fs::create_directories(destNative.parent_path(), ec);
                        fs::copy_file(it->path(), destNative,
                            fs::copy_options::overwrite_existing, ec);
                        // Best-effort; a copy failure shouldn't fail the build.
                    }
                };

                packagemanager::MtModulesManager modulesMgr(config.projectRoot);

                for (const auto& dep : config.dependencies.packages) {
                    // Source-distributed deps live in mt_modules/@<name>/ and
                    // don't have a .mtcLib — the compiler pulls source in via
                    // alias-driven imports (see compileToProgram). The native
                    // binaries the user's code references at runtime still
                    // need to ship next to the exe.
                    if (modulesMgr.isInstalled(dep.name)) {
                        fs::path pkgRoot = fs::path(config.projectRoot)
                            / "mt_modules" / ("@" + dep.name);
                        mirrorNativeBinaries(pkgRoot);
                        continue;
                    }

                    // Binary-distributed dep: copy the .mtcLib and any native
                    // binaries that sit alongside it in the package directory.
                    auto resolved = linker.getPathResolver().resolve(dep.name);
                    if (!resolved) continue;

                    fs::path destLib = libsDir / (dep.name + ".mtcLib");
                    fs::copy_file(*resolved, destLib,
                        fs::copy_options::overwrite_existing);

                    std::error_code ec;
                    fs::path resolvedCanonical = fs::weakly_canonical(fs::path(*resolved), ec);
                    if (ec) continue;
                    mirrorNativeBinaries(resolvedCanonical.parent_path());
                }
            }

            result.filesCompiled = sourceFiles.size();
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());

            // Remove partially written output so we don't leave a corrupt exe.
            try { std::filesystem::remove(outputPath); } catch (...) {}
        }

        auto endTime = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return result;
    }
}
