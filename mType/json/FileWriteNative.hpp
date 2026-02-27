#pragma once
#include <string>

namespace json
{
    class FileWriteNative
    {
    public:
        static void writeFile(const std::string& filePath,
                             const std::string& content);

    private:
        static void validateFilePath(const std::string& filePath);
    };
}
