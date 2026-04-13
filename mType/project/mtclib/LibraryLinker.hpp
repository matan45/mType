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

        // Get the path resolver (for testing/configuration)
        MtcPathResolver& getPathResolver() { return pathResolver; }

    private:
        MtcPathResolver pathResolver;
        DependencyResolver dependencyResolver;
        std::unordered_map<std::string, MtcLibProgram> loadedLibraries;

        // Recursively load a library and its transitive dependencies
        void loadLibraryRecursive(const std::string& libraryName, const std::string& versionConstraint);
    };
}
