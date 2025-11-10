#pragma once

#include "../../ast/VisibilityModifier.hpp"
#include "../../value/ValueType.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <optional>

namespace environment::registry
{
    using namespace ast;
    using namespace value;

    /**
     * @brief Type of exported symbol
     */
    enum class ExportedSymbolType
    {
        CLASS,
        INTERFACE,
        FUNCTION,
        VARIABLE
    };

    /**
     * @brief Information about an exported symbol
     */
    struct ExportedSymbol
    {
        std::string name;
        ExportedSymbolType type;
        VisibilityModifier visibility;
        std::string sourceFile;  // File where the symbol is defined

        // Default constructor for std::unordered_map
        ExportedSymbol()
            : name(""), type(ExportedSymbolType::CLASS),
              visibility(VisibilityModifier::PUBLIC), sourceFile("") {}

        ExportedSymbol(const std::string& n, ExportedSymbolType t,
                      VisibilityModifier v, const std::string& file)
            : name(n), type(t), visibility(v), sourceFile(file) {}

        bool isPublic() const { return visibility == VisibilityModifier::PUBLIC; }
        bool isPrivate() const { return visibility == VisibilityModifier::PRIVATE; }
    };

    /**
     * @brief Registry for tracking exported symbols across files
     *
     * This class manages the visibility and accessibility of symbols across
     * different files in the import/export system.
     */
    class ExportRegistry
    {
    private:
        // Map: file_path -> symbol_name -> ExportedSymbol
        std::unordered_map<std::string, std::unordered_map<std::string, ExportedSymbol>> fileExports;

        // Quick lookup: symbol_name -> [file_paths]
        std::unordered_map<std::string, std::unordered_set<std::string>> symbolToFiles;

    public:
        ExportRegistry() = default;
        ~ExportRegistry() = default;

        /**
         * @brief Register an exported symbol
         * @param filePath The file where the symbol is defined
         * @param symbolName The name of the symbol
         * @param symbolType The type of the symbol (class, function, etc.)
         * @param visibility The visibility modifier (public/private)
         */
        void registerSymbol(const std::string& filePath,
                           const std::string& symbolName,
                           ExportedSymbolType symbolType,
                           VisibilityModifier visibility);

        /**
         * @brief Check if a symbol is exported from a file
         * @param filePath The file to check
         * @param symbolName The symbol name to check
         * @return true if the symbol is exported (public), false otherwise
         */
        bool isSymbolExported(const std::string& filePath, const std::string& symbolName) const;

        /**
         * @brief Check if a symbol exists in a file (regardless of visibility)
         * @param filePath The file to check
         * @param symbolName The symbol name to check
         * @return true if the symbol exists, false otherwise
         */
        bool symbolExists(const std::string& filePath, const std::string& symbolName) const;

        /**
         * @brief Get the visibility of a symbol
         * @param filePath The file where the symbol is defined
         * @param symbolName The symbol name
         * @return The visibility modifier, or nullopt if not found
         */
        std::optional<VisibilityModifier> getSymbolVisibility(const std::string& filePath,
                                                              const std::string& symbolName) const;

        /**
         * @brief Get all public symbols from a file
         * @param filePath The file to get exports from
         * @return Vector of exported symbol names
         */
        std::vector<std::string> getPublicSymbols(const std::string& filePath) const;

        /**
         * @brief Get all symbols from a file (including private)
         * @param filePath The file to get symbols from
         * @return Vector of all symbol names
         */
        std::vector<std::string> getAllSymbols(const std::string& filePath) const;

        /**
         * @brief Get detailed information about a symbol
         * @param filePath The file where the symbol is defined
         * @param symbolName The symbol name
         * @return Pointer to ExportedSymbol if found, nullptr otherwise
         */
        const ExportedSymbol* getSymbolInfo(const std::string& filePath,
                                           const std::string& symbolName) const;

        /**
         * @brief Clear all exports for a file (useful for hot reload)
         * @param filePath The file to clear exports for
         */
        void clearFileExports(const std::string& filePath);

        /**
         * @brief Clear all exports
         */
        void clearAll();

        /**
         * @brief Get all files that export a particular symbol
         * @param symbolName The symbol name
         * @return Set of file paths
         */
        std::unordered_set<std::string> getFilesExportingSymbol(const std::string& symbolName) const;
    };
}
