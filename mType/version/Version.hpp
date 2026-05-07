#pragma once
#include <string>

// Single-source version constants for native mType tools.
// CLI tools, the package manager, language server metadata, and .mtcLib
// metadata should read from here so one edit propagates through native builds.
namespace mType::version
{
    inline constexpr int MAJOR = 1;
    inline constexpr int MINOR = 0;
    inline constexpr int PATCH = 0;

    // Returns "MAJOR.MINOR.PATCH", the canonical user-facing form used by
    // --version, .mtcLib metadata, the LSP serverInfo response, and docs.
    // Cached in a function-local static so the concatenation runs once.
    inline const std::string& getVersionString()
    {
        static const std::string s =
            std::to_string(MAJOR) + "." +
            std::to_string(MINOR) + "." +
            std::to_string(PATCH);
        return s;
    }
}
