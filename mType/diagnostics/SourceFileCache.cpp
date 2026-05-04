#include "SourceFileCache.hpp"
#include <sstream>
#include <mutex>

namespace diagnostics
{
    SourceFileCache& SourceFileCache::instance()
    {
        static SourceFileCache cache;
        return cache;
    }

    std::string SourceFileCache::normalizeFilename(const std::string& filename)
    {
        std::string out;
        out.reserve(filename.size());
        char last = 0;
        for (char c : filename)
        {
            char normalized = (c == '\\') ? '/' : c;
            // Collapse runs of '/'
            if (normalized == '/' && last == '/')
            {
                continue;
            }
            out.push_back(normalized);
            last = normalized;
        }
        return out;
    }

    std::vector<std::string> SourceFileCache::splitLines(const std::string& content)
    {
        std::vector<std::string> lines;
        std::string current;
        current.reserve(80);
        for (char c : content)
        {
            if (c == '\n')
            {
                // Strip trailing '\r' for CRLF inputs
                if (!current.empty() && current.back() == '\r')
                {
                    current.pop_back();
                }
                lines.push_back(std::move(current));
                current.clear();
            }
            else
            {
                current.push_back(c);
            }
        }
        if (!current.empty())
        {
            if (current.back() == '\r')
            {
                current.pop_back();
            }
            lines.push_back(std::move(current));
        }
        return lines;
    }

    void SourceFileCache::publish(const std::string& filename, std::vector<std::string> lines)
    {
        const std::string key = normalizeFilename(filename);
        std::unique_lock lock(mutex_);
        files_[key] = std::move(lines);
    }

    void SourceFileCache::publishFromContent(const std::string& filename, const std::string& content)
    {
        publish(filename, splitLines(content));
    }

    std::optional<std::string> SourceFileCache::getLine(const std::string& filename, int line1Based) const
    {
        if (line1Based < 1)
        {
            return std::nullopt;
        }
        const std::string key = normalizeFilename(filename);
        std::shared_lock lock(mutex_);
        auto it = files_.find(key);
        if (it == files_.end())
        {
            return std::nullopt;
        }
        const auto& lines = it->second;
        const size_t index = static_cast<size_t>(line1Based - 1);
        if (index >= lines.size())
        {
            return std::nullopt;
        }
        return lines[index];
    }

    size_t SourceFileCache::getLineCount(const std::string& filename) const
    {
        const std::string key = normalizeFilename(filename);
        std::shared_lock lock(mutex_);
        auto it = files_.find(key);
        return (it == files_.end()) ? 0u : it->second.size();
    }

    void SourceFileCache::invalidate(const std::string& filename)
    {
        const std::string key = normalizeFilename(filename);
        std::unique_lock lock(mutex_);
        files_.erase(key);
    }

    void SourceFileCache::clear()
    {
        std::unique_lock lock(mutex_);
        files_.clear();
    }
}
