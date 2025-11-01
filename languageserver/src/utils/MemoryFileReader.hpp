#pragma once

#include <string>
#include <unordered_map>
#include "../../../mType/services/FileReader.hpp"

namespace mtype::lsp {

/**
 * @brief FileReader implementation that reads from in-memory content
 *
 * This is used by the LSP to provide content directly without
 * needing to write temporary files to disk.
 */
class MemoryFileReader : public lexer::FileReader {
public:
    MemoryFileReader() = default;

    /**
     * @brief Set content for a given URI
     */
    void setContent(const std::string& uri, const std::string& content) {
        contentMap_[uri] = content;
    }

    /**
     * @brief Read file content - returns stored content for LSP documents
     */
    std::string readFile(const std::string& filePath) override {
        auto it = contentMap_.find(filePath);
        if (it != contentMap_.end()) {
            return it->second;
        }

        // If not found in memory, try to read from actual file
        return lexer::FileReader::readFile(filePath);
    }

private:
    std::unordered_map<std::string, std::string> contentMap_;
};

} // namespace mtype::lsp
