#pragma once
#include "MtcLibFormat.hpp"
#include "MtcPathResolver.hpp"
#include "DependencyResolver.hpp"
#include "../ProjectConfig.hpp"
#include "../../environment/Environment.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace project::mtclib
{
    /**
     * Orchestrates compile-time library linking.
     * Resolves library paths, deserializes .mtcLib files,
     * resolves transitive dependencies, and returns them in load order.
     */
    class LibraryLinker
    {
    public:
        explicit LibraryLinker(const std::string& projectRoot = ".");

        // Resolve all dependencies from project config, returning libraries in load order
        std::vector<MtcLibProgram> linkDependencies(const ProjectConfig& config);

        // Add additional search path for library resolution
        void addSearchPath(const std::string& path);

        // Override declared version ranges with lockfile-pinned versions.
        // Caller passes a map of <name -> exact version>. Names absent from the
        // map keep their declared range from the .mtproj.
        void setLockfileVersions(const std::unordered_map<std::string, std::string>& versions);

        // Get the path resolver (for testing/configuration)
        MtcPathResolver& getPathResolver() { return pathResolver; }

    private:
        MtcPathResolver pathResolver;
        DependencyResolver dependencyResolver;
        std::unordered_map<std::string, MtcLibProgram> loadedLibraries;
        std::unordered_map<std::string, std::string> lockedVersions;

        // Recursively load a library and its transitive dependencies
        void loadLibraryRecursive(const std::string& libraryName, const std::string& versionConstraint);

        // Build the user-facing error message when a library can't be resolved.
        [[noreturn]] void throwLibraryNotFound(const std::string& libraryName) const;
    };
}
