#include "ExportRegistry.hpp"
#include <algorithm>

namespace environment::registry
{
    void ExportRegistry::registerSymbol(const std::string& filePath,
                                       const std::string& symbolName,
                                       ExportedSymbolType symbolType,
                                       VisibilityModifier visibility)
    {
        ExportedSymbol symbol(symbolName, symbolType, visibility, filePath);
        fileExports[filePath][symbolName] = symbol;

        // Update reverse lookup
        symbolToFiles[symbolName].insert(filePath);
    }

    bool ExportRegistry::isSymbolExported(const std::string& filePath,
                                         const std::string& symbolName) const
    {
        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return false;
        }

        auto symbolIt = fileIt->second.find(symbolName);
        if (symbolIt == fileIt->second.end())
        {
            return false;
        }

        // Only public symbols are exported
        return symbolIt->second.isPublic();
    }

    bool ExportRegistry::symbolExists(const std::string& filePath,
                                      const std::string& symbolName) const
    {
        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return false;
        }

        return fileIt->second.find(symbolName) != fileIt->second.end();
    }

    std::optional<VisibilityModifier> ExportRegistry::getSymbolVisibility(
        const std::string& filePath,
        const std::string& symbolName) const
    {
        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return std::nullopt;
        }

        auto symbolIt = fileIt->second.find(symbolName);
        if (symbolIt == fileIt->second.end())
        {
            return std::nullopt;
        }

        return symbolIt->second.visibility;
    }

    std::vector<std::string> ExportRegistry::getPublicSymbols(const std::string& filePath) const
    {
        std::vector<std::string> publicSymbols;

        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return publicSymbols;
        }

        for (const auto& [symbolName, symbolInfo] : fileIt->second)
        {
            if (symbolInfo.isPublic())
            {
                publicSymbols.push_back(symbolName);
            }
        }

        // Sort for consistent ordering
        std::sort(publicSymbols.begin(), publicSymbols.end());
        return publicSymbols;
    }

    std::vector<std::string> ExportRegistry::getAllSymbols(const std::string& filePath) const
    {
        std::vector<std::string> allSymbols;

        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return allSymbols;
        }

        for (const auto& [symbolName, _] : fileIt->second)
        {
            allSymbols.push_back(symbolName);
        }

        // Sort for consistent ordering
        std::sort(allSymbols.begin(), allSymbols.end());
        return allSymbols;
    }

    const ExportedSymbol* ExportRegistry::getSymbolInfo(const std::string& filePath,
                                                       const std::string& symbolName) const
    {
        auto fileIt = fileExports.find(filePath);
        if (fileIt == fileExports.end())
        {
            return nullptr;
        }

        auto symbolIt = fileIt->second.find(symbolName);
        if (symbolIt == fileIt->second.end())
        {
            return nullptr;
        }

        return &symbolIt->second;
    }

    void ExportRegistry::clearFileExports(const std::string& filePath)
    {
        auto fileIt = fileExports.find(filePath);
        if (fileIt != fileExports.end())
        {
            // Remove from reverse lookup
            for (const auto& [symbolName, _] : fileIt->second)
            {
                auto symbolFilesIt = symbolToFiles.find(symbolName);
                if (symbolFilesIt != symbolToFiles.end())
                {
                    symbolFilesIt->second.erase(filePath);
                    if (symbolFilesIt->second.empty())
                    {
                        symbolToFiles.erase(symbolFilesIt);
                    }
                }
            }

            fileExports.erase(fileIt);
        }
    }

    void ExportRegistry::clearAll()
    {
        fileExports.clear();
        symbolToFiles.clear();
    }

    std::unordered_set<std::string> ExportRegistry::getFilesExportingSymbol(
        const std::string& symbolName) const
    {
        auto it = symbolToFiles.find(symbolName);
        if (it != symbolToFiles.end())
        {
            return it->second;
        }
        return {};
    }
}
