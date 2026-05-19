#include "ProjectBuilder.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include "mtclib/MtcLibBuilder.hpp"
#include "mtclib/MtcLibSerializer.hpp"
#include "mtclib/ContentHash.hpp"
#include "../environment/EnvironmentBuilder.hpp"

namespace project
{
    BuildResult ProjectBuilder::buildLibrary(const ProjectConfig& config, const std::string& outputPath)
    {
        environment::EnvironmentBuilder envBuilder;
        auto environment = envBuilder.build();
        return buildLibrary(config, outputPath, environment);
    }

    BuildResult ProjectBuilder::buildLibrary(const ProjectConfig& config, const std::string& outputPath,
                                              std::shared_ptr<environment::Environment> environment)
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

        try
        {
            // Incremental build: skip recompilation if source files haven't changed.
            uint64_t currentSourceHash = mtclib::ContentHash::hashFiles(sourceFiles);
            if (std::filesystem::exists(outputPath))
            {
                std::ifstream existingFile(outputPath, std::ios::binary);
                if (existingFile.good())
                {
                    try
                    {
                        auto existingLib = mtclib::MtcLibSerializer::deserialize(existingFile);
                        existingFile.close();
                        if (existingLib.metadata.sourceHash == currentSourceHash && currentSourceHash != 0)
                        {
                            // Source unchanged — skip rebuild.
                            result.filesCompiled = 0;
                            auto endTime = std::chrono::steady_clock::now();
                            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                            return result;
                        }
                    }
                    catch (...)
                    {
                        // Existing file is corrupt or incompatible — rebuild.
                    }
                }
            }

            reportProgress(1, 1, "Building library...");

            auto program = compileToProgram(config, environment);

            std::filesystem::path outPath(outputPath);
            if (outPath.has_parent_path() && !std::filesystem::exists(outPath.parent_path()))
            {
                std::filesystem::create_directories(outPath.parent_path());
            }

            auto exportRegistry = environment->getExportRegistry();
            auto mtcLib = mtclib::MtcLibBuilder::build(
                program, config, exportRegistry ? *exportRegistry : environment::registry::ExportRegistry{});

            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile)
            {
                throw std::runtime_error("Could not create output file: " + outputPath);
            }
            mtclib::MtcLibSerializer::serialize(mtcLib, outFile);
            outFile.close();

            result.filesCompiled = sourceFiles.size();
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());
        }

        auto endTime = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return result;
    }
}
