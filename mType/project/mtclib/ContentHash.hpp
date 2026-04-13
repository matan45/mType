#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>

namespace project::mtclib
{
    /**
     * FNV-1a 64-bit hash for deterministic, cross-platform content hashing.
     * Used for source file integrity (sourceHash) and dependency validation (contentHash).
     */
    class ContentHash
    {
    public:
        // FNV-1a over a byte range
        static uint64_t hashBytes(const char* data, size_t len)
        {
            uint64_t hash = FNV_OFFSET_BASIS;
            for (size_t i = 0; i < len; ++i)
            {
                hash ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
                hash *= FNV_PRIME;
            }
            return hash;
        }

        // Hash the contents of a single file.
        // Returns 0 if file cannot be read (caller should treat 0 as "hash unavailable").
        static uint64_t hashFile(const std::string& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file) return 0;  // File not found or unreadable — sentinel value

            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            return hashBytes(content.data(), content.size());
        }

        // Hash multiple files in order (for sourceHash — files should be pre-sorted)
        static uint64_t hashFiles(const std::vector<std::string>& sortedPaths)
        {
            uint64_t hash = FNV_OFFSET_BASIS;
            for (const auto& path : sortedPaths)
            {
                std::ifstream file(path, std::ios::binary);
                if (!file) continue;

                // Mix in the file path (so identical content in different files produces different hashes)
                for (char c : path)
                {
                    hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
                    hash *= FNV_PRIME;
                }

                // Mix in the file content
                char buf[4096];
                while (file.read(buf, sizeof(buf)) || file.gcount() > 0)
                {
                    size_t bytesRead = static_cast<size_t>(file.gcount());
                    for (size_t i = 0; i < bytesRead; ++i)
                    {
                        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(buf[i]));
                        hash *= FNV_PRIME;
                    }
                }
            }
            return hash;
        }

    private:
        static constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
        static constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;
    };
}
