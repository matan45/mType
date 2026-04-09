#pragma once

#include <string>
#include <sstream>
#include <filesystem>

namespace mtype::lsp {

/**
 * Shared URI/path utility functions for the language server.
 */
namespace UriUtils {

    /**
     * URL decode a string (e.g., %20 -> space, %3A -> :)
     */
    inline std::string urlDecode(const std::string& str)
    {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i)
        {
            if (str[i] == '%' && i + 2 < str.size())
            {
                int value;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> value)
                {
                    result += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    result += str[i];
                }
            }
            else if (str[i] == '+')
            {
                result += ' ';
            }
            else
            {
                result += str[i];
            }
        }

        return result;
    }

    /**
     * Convert a file:// URI to a filesystem path.
     * Handles URL decoding and Windows drive letter prefix.
     */
    inline std::string uriToFilePath(const std::string& uri)
    {
        std::string path = uri;
        const std::string filePrefix = "file:///";

        if (path.find(filePrefix) == 0)
        {
            path = path.substr(filePrefix.length());
            path = urlDecode(path);

            // On Windows, convert /C:/path to C:/path
            if (path.length() >= 3 && path[0] == '/' && path[2] == ':')
            {
                path = path.substr(1);
            }
        }

        return path;
    }

} // namespace UriUtils
} // namespace mtype::lsp
