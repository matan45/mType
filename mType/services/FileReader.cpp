#include "FileReader.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <array>
#include "../errors/FileException.hpp"

namespace lexer
{
    std::string FileReader::readFile(const std::string& filePath)
    {
        validateFile(filePath);
        
        const size_t fileSize = getFileSize(filePath);
        
        // Choose optimal reading strategy based on file size
        if (fileSize <= LARGE_FILE_THRESHOLD)
        {
            return readSmallFile(filePath);
        }
        else
        {
            return readLargeFileBuffered(filePath, DEFAULT_BUFFER_SIZE);
        }
    }

    std::string FileReader::readFileBuffered(const std::string& filePath, size_t bufferSize)
    {
        validateFile(filePath);
        return readLargeFileBuffered(filePath, bufferSize);
    }

    void FileReader::streamFile(const std::string& filePath,
                               std::function<void(std::string_view)> chunkProcessor,
                               size_t bufferSize)
    {
        validateFile(filePath);

        std::ifstream file = openFileForReading(filePath);

        // Handle UTF-8 BOM for streaming
        const bool hadBom = checkAndSkipUtf8Bom(file);

        // Create buffer for streaming
        std::vector<char> buffer(bufferSize);
        bool isFirstChunk = true;

        while (file.good() && !file.eof())
        {
            file.read(buffer.data(), static_cast<std::streamsize>(bufferSize));
            const std::streamsize bytesRead = file.gcount();

            if (bytesRead > 0)
            {
                std::string_view chunk(buffer.data(), static_cast<size_t>(bytesRead));

                // Handle BOM removal for first chunk if we had one
                if (isFirstChunk)
                {
                    chunk = removeBomFromChunk(chunk, hadBom);
                }

                if (!chunk.empty())
                {
                    chunkProcessor(chunk);
                }
                isFirstChunk = false;
            }
        }

        file.close();
    }

    std::string FileReader::readSmallFile(const std::string& filePath)
    {
        std::ifstream file = openFileForReading(filePath);

        // For small files, use the original efficient approach
        file.seekg(0, std::ios::end);
        const auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content;
        content.reserve(static_cast<size_t>(fileSize));

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();

        file.close();

        return removeUtf8BomIfPresent(content);
    }

    std::string FileReader::readLargeFileBuffered(const std::string& filePath, size_t bufferSize)
    {
        std::ifstream file = openFileForReading(filePath);

        const size_t fileSize = getFileSize(filePath);
        std::string content;
        content.reserve(fileSize);

        // Handle UTF-8 BOM
        const bool hadBom = checkAndSkipUtf8Bom(file);

        // Use buffered reading for large files
        std::vector<char> buffer(bufferSize);
        bool isFirstChunk = true;

        while (file.good() && !file.eof())
        {
            file.read(buffer.data(), static_cast<std::streamsize>(bufferSize));
            const std::streamsize bytesRead = file.gcount();

            if (bytesRead > 0)
            {
                std::string_view chunk(buffer.data(), static_cast<size_t>(bytesRead));

                // Handle BOM removal for first chunk
                if (isFirstChunk)
                {
                    chunk = removeBomFromChunk(chunk, hadBom);
                }

                content.append(chunk);
                isFirstChunk = false;
            }
        }

        file.close();
        return content;
    }

    void FileReader::validateFile(const std::string& filePath) const
    {
        if (filePath.empty())
        {
            throw errors::FileException("File path cannot be empty");
        }

        if (!std::filesystem::exists(filePath))
        {
            throw errors::FileException("File does not exist: " + filePath);
        }

        if (!std::filesystem::is_regular_file(filePath))
        {
            throw errors::FileException("Path is not a regular file: " + filePath);
        }

        // Check file size (avoid loading extremely large files)
        const auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize > MAX_FILE_SIZE)
        {
            throw errors::FileException("File too large (>100MB): " + filePath);
        }
    }

    size_t FileReader::getFileSize(const std::string& filePath) const
    {
        return static_cast<size_t>(std::filesystem::file_size(filePath));
    }

    std::string FileReader::removeUtf8BomIfPresent(const std::string& content) const
    {
        if (content.length() >= UTF8_BOM.length() &&
            content.compare(0, UTF8_BOM.length(), UTF8_BOM) == 0)
        {
            return content.substr(UTF8_BOM.length());
        }
        return content;
    }

    bool FileReader::checkAndSkipUtf8Bom(std::ifstream& file) const
    {
        // Save current position
        const auto originalPos = file.tellg();

        // Read potential BOM
        std::array<char, 3> bomBuffer{};
        file.read(bomBuffer.data(), 3);
        const std::streamsize bytesRead = file.gcount();

        // Check if we have a UTF-8 BOM
        if (bytesRead == 3 &&
            std::string_view(bomBuffer.data(), static_cast<size_t>(bytesRead)) == UTF8_BOM)
        {
            // BOM found, leave position after BOM
            return true;
        }
        else
        {
            // No BOM, reset to original position
            file.seekg(originalPos);
            return false;
        }
    }

    std::string_view FileReader::removeBomFromChunk(std::string_view chunk, bool hadBom) const
    {
        if (hadBom && chunk.size() >= UTF8_BOM.length())
        {
            if (chunk.substr(0, UTF8_BOM.length()) == UTF8_BOM)
            {
                return chunk.substr(UTF8_BOM.length());
            }
        }
        return chunk;
    }

    std::ifstream FileReader::openFileForReading(const std::string& filePath) const
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            throw errors::FileException("Cannot open file: " + filePath);
        }
        return file;
    }
}