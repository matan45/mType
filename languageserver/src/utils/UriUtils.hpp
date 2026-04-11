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

    /**
     * Convert a filesystem path to a `file://` URI. Used by the
     * diagnostic converter when a secondary span references a file
     * outside the document the LSP is currently publishing diagnostics
     * for. Best-effort: encodes the characters that would silently
     * break URI parsing (`#` truncates to a fragment, `?` to a query
     * string, `%` is itself the escape introducer, and `[` / `]` are
     * reserved for the host component) plus spaces. Other URI-reserved
     * characters aren't filename-legal on Windows so they're skipped.
     * Normalises Windows backslashes to forward slashes.
     */
    inline std::string filePathToUri(const std::string& path)
    {
        if (path.empty())
        {
            return "";
        }
        std::string normalized;
        normalized.reserve(path.size());
        for (char c : path)
        {
            normalized.push_back(c == '\\' ? '/' : c);
        }
        std::string encoded;
        encoded.reserve(normalized.size());
        auto appendHex = [&encoded](unsigned char c) {
            static const char* kHex = "0123456789ABCDEF";
            encoded.push_back('%');
            encoded.push_back(kHex[(c >> 4) & 0xF]);
            encoded.push_back(kHex[c & 0xF]);
        };
        for (char c : normalized)
        {
            switch (c)
            {
                case ' ':
                case '#':
                case '?':
                case '%':
                case '[':
                case ']':
                    appendHex(static_cast<unsigned char>(c));
                    break;
                default:
                    encoded.push_back(c);
                    break;
            }
        }
        // file:///C:/foo  on Windows;  file:///home/x/foo  on POSIX
        if (!encoded.empty() && encoded.front() == '/')
        {
            return "file://" + encoded;
        }
        return "file:///" + encoded;
    }

} // namespace UriUtils
} // namespace mtype::lsp
