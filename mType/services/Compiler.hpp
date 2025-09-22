#pragma once
#include <string>

namespace services
{
    /**
     * Pure compilation service - stateless and isolated
     * Converts .mt source files to .mtc compiled files
     */
    class Compiler
    {
    public:
        Compiler() = default;
        ~Compiler() = default;

        /**
         * Compile a source file to cached AST format
         * This operation is completely stateless and isolated
         * @param sourceFile Path to .mt source file
         * @param outputFile Path to .mtc output file (optional)
         * @return true if compilation successful, false otherwise
         */
        bool compile(const std::string& sourceFile, const std::string& outputFile = "");

    private:
        // No member variables - completely stateless
    };
}