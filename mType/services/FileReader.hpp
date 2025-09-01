#pragma once
#include <string>
#include <memory>
#include <fstream>
#include <functional>

namespace lexer
{
    /**
     * File reader with buffered reading and streaming capabilities
     * Optimized for memory usage with large files
     */
    class FileReader
    {
    public:
        // Configuration constants
        static constexpr size_t DEFAULT_BUFFER_SIZE = 8192;  // 8KB buffer
        static constexpr size_t LARGE_FILE_THRESHOLD = 10 * 1024 * 1024; // 10MB
        static constexpr size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB limit
        
        // Main interface - automatically chooses optimal strategy
        std::string readFile(const std::string& filePath);
        
        // Advanced interface for custom buffer sizes
        std::string readFileBuffered(const std::string& filePath, size_t bufferSize = DEFAULT_BUFFER_SIZE);
        
        // Streaming interface for processing large files chunk by chunk
        void streamFile(const std::string& filePath, 
                       std::function<void(std::string_view)> chunkProcessor,
                       size_t bufferSize = DEFAULT_BUFFER_SIZE);

    private:
        // File validation and metadata
        void validateFile(const std::string& filePath) const;
        size_t getFileSize(const std::string& filePath) const;
        
        // Reading strategies
        std::string readSmallFile(const std::string& filePath);
        std::string readLargeFileBuffered(const std::string& filePath, size_t bufferSize);
        
        // UTF-8 BOM handling
        std::string removeUtf8BomIfPresent(const std::string& content) const;
        bool checkAndSkipUtf8Bom(std::ifstream& file) const;
        
        // Buffer management
        std::string createOptimalBuffer(size_t size) const;
        
        static constexpr std::string_view UTF8_BOM = "\xEF\xBB\xBF";
    };
}