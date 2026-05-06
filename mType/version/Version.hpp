#pragma once
#include <string>

// Single-source version constants for the mType compiler/runtime.
//
// IMPORTANT: when bumping the version, also update:
//   * languageserver/src/MTypeLanguageServer.cpp — `serverInfo.version` JSON
//     field (separate build target — does not include this header).
//   * README.md — the version shield badge.
//
// Anything inside the mType/ tree (mtype-core, mtype-vm, mtype-runtime,
// mtype-cli, etc.) should read from here so a single edit propagates.
namespace mType::version
{
    inline constexpr int MAJOR = 0;
    inline constexpr int MINOR = 2;
    inline constexpr int PATCH = 0;

    // Returns "MAJOR.MINOR.PATCH" — the canonical user-facing form used by
    // --version, the .mtcLib metadata, the LSP serverInfo response, and the
    // README badge.
    inline std::string getVersionString()
    {
        return std::to_string(MAJOR) + "." +
               std::to_string(MINOR) + "." +
               std::to_string(PATCH);
    }
}
