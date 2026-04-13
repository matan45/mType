#pragma once
#include "MtcLibFormat.hpp"
#include "MtcPathResolver.hpp"
#include "DependencyResolver.hpp"
#include "../../vm/runtime/LibraryLoader.hpp"
#include "../../environment/Environment.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace project::mtclib
{
    /**
     * Loads .mtcLib files with full transitive dependency resolution.
     * Composes LibraryLoader (single-library loading), DependencyResolver
     * (topological sort + cycle detection), and MtcPathResolver (path lookup).
     *
     * Lives in mtype-extensions because DependencyResolver and MtcPathResolver
     * are not accessible from mtype-vm where LibraryLoader resides.
     */
    class TransitiveDependencyLoader
    {
    public:
        explicit TransitiveDependencyLoader(const std::string& projectRoot = ".");

        // Load a single .mtcLib plus all its transitive dependencies in correct order
        void loadLibraryWithDependencies(
            const std::string& path,
            vm::runtime::VirtualMachine& vm,
            std::shared_ptr<environment::Environment> env);

        // Load multiple .mtcLib files plus all transitive dependencies in correct order
        void loadLibrariesWithDependencies(
            const std::vector<std::string>& paths,
            vm::runtime::VirtualMachine& vm,
            std::shared_ptr<environment::Environment> env);

        // Add additional search path for transitive dependency resolution
        void addSearchPath(const std::string& path);

        // Get the path resolver (for configuration)
        MtcPathResolver& getPathResolver() { return pathResolver; }

    private:
        MtcPathResolver pathResolver;
        DependencyResolver dependencyResolver;
        vm::runtime::LibraryLoader libraryLoader;

        // Owns loaded MtcLibPrograms to keep BytecodeProgram pointers alive.
        // The VM stores raw pointers to BytecodePrograms, so these must outlive
        // VM execution. Stored here rather than in LibraryLoader because the
        // reference-based loadLibrary overload does not take ownership.
        std::vector<std::unique_ptr<MtcLibProgram>> ownedPrograms;

        // Collect a library and its transitive deps into the map (by name)
        void collectDependencies(
            const std::string& libraryName,
            const std::string& versionConstraint,
            std::unordered_map<std::string, MtcLibProgram>& collected,
            std::shared_ptr<environment::Environment> env);

        // Load collected libraries in topological order via DependencyResolver
        void loadInOrder(
            std::unordered_map<std::string, MtcLibProgram>& collected,
            vm::runtime::VirtualMachine& vm,
            std::shared_ptr<environment::Environment> env);
    };
}
