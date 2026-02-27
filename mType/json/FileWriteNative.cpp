#include "FileWriteNative.hpp"
#include "../errors/RuntimeException.hpp"
#include <fstream>
#include <filesystem>

namespace json
{
    void FileWriteNative::writeFile(const std::string& filePath,
                                    const std::string& content)
    {
        validateFilePath(filePath);

        auto parentPath = std::filesystem::path(filePath).parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath))
        {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            throw errors::RuntimeException(
                "Cannot open file for writing: " + filePath);
        }

        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file.good())
        {
            throw errors::RuntimeException(
                "Error writing to file: " + filePath);
        }

        file.close();
    }

    void FileWriteNative::validateFilePath(const std::string& filePath)
    {
        if (filePath.empty())
        {
            throw errors::RuntimeException("File path cannot be empty");
        }
    }
}
