#pragma once
#include "DependencyResolver.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace packagemanager
{
    using CompileProgressCallback = std::function<void(const std::string& message)>;

    /**
     * Compiles installed registry packages into .mtcLib bytecode artifacts.
     *
     * For each resolved package, synthesizes a .mtproj rooted at the package's
     * registry dir and invokes `mType.exe --build --lib`. The compiled
     * <name>.mtcLib lands at:
     *   <registry>/<name>/<version>/<name>.mtcLib
     *
     * Compilation runs in topological order so that a package that depends on
     * another sees the dependency's .mtcLib already on disk when its own
     * compile step runs through LibraryLinker.
     */
    class PackageCompiler
    {
    public:
        PackageCompiler();

        // Path to mType.exe used for the subprocess invocation. If empty,
        // discovery falls back to PATH lookup.
        void setMTypeExecutable(const std::string& path);

        void setProgressCallback(CompileProgressCallback callback);

        struct CompileResult
        {
            bool success = true;
            std::vector<std::string> errors;
            size_t compiled = 0;
            size_t skipped = 0;
        };

        // Compile every package in `resolved` to a .mtcLib in its registry
        // version dir. Packages whose <name>.mtcLib already exists and is newer
        // than every source file are skipped.
        CompileResult compileAll(
            const std::unordered_map<std::string, ResolvedPackage>& resolved);

        // Discover mType.exe relative to a candidate mtpm.exe path. Returns
        // empty string if no usable path was found (caller may still fall back
        // to PATH).
        static std::string discoverMTypeExecutable(const std::string& mtpmExePath);

        // Topological sort of resolved packages: deps before dependents.
        // Exposed for unit testing; safe to call standalone.
        static std::vector<std::string> topologicalOrder(
            const std::unordered_map<std::string, ResolvedPackage>& resolved);

        // Generate the .mtproj XML for a package. Exposed for unit testing.
        static std::string buildSyntheticMtproj(
            const ResolvedPackage& pkg,
            const std::string& sourceInclude,
            const std::unordered_map<std::string, ResolvedPackage>& resolved);

    private:
        std::string mTypeExePath;
        CompileProgressCallback progressCallback;

        void reportProgress(const std::string& message);

        // Compile one package. Returns true on success (or no-op skip).
        bool compileOne(const ResolvedPackage& pkg,
                        const std::unordered_map<std::string, ResolvedPackage>& resolved,
                        std::string& error);

        // Decide whether <pkg>.mtcLib is up to date relative to source files.
        bool isUpToDate(const ResolvedPackage& pkg,
                        const std::string& sourceDir) const;

        // Run mType.exe --build --lib <mtprojPath>. Returns exit code; captures
        // stdout+stderr into `output`.
        int runMTypeBuild(const std::string& mtprojPath, std::string& output) const;
    };
}
