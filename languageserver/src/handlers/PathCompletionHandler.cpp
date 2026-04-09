#include "PathCompletionHandler.hpp"
#include "../utils/UriUtils.hpp"
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace mtype::lsp
{
    PathCompletionHandler::PathCompletionHandler(DocumentManager* docMgr)
        : documentManager_(docMgr)
    {
    }

    std::vector<CompletionItem> PathCompletionHandler::getPathCompletions(
        const std::string& uri,
        const Position& position
    )
    {
        std::vector<CompletionItem> items;

        auto doc = documentManager_->getDocument(uri);
        if (!doc)
        {
            return items;
        }

        // Check if we're inside an import string
        bool insideImport = isInsideImportString(doc->content, position);

        if (!insideImport)
        {
            return items;
        }

        // Extract the current path being typed
        std::string currentPath = extractImportPath(doc->content, position);

        // Resolve the path relative to the current file
        std::string resolvedPath = resolveRelativePath(uri, currentPath);

        // List directory contents
        auto completions = listDirectoryContents(resolvedPath);

        return completions;
    }

    bool PathCompletionHandler::isInsideImportString(const std::string& content, const Position& position)
    {
        std::istringstream stream(content);
        std::string line;
        int currentLine = 0;

        // Get the line at the cursor position
        while (std::getline(stream, line) && currentLine < position.line)
        {
            currentLine++;
        }

        if (currentLine != position.line)
        {
            return false;
        }

        // Check if line contains "import" and cursor is inside quotes
        if (line.find("import") == std::string::npos)
        {
            return false;
        }

        // Count quotes before cursor position
        int quoteCount = 0;
        for (int i = 0; i < position.character && i < static_cast<int>(line.length()); i++)
        {
            if (line[i] == '"')
            {
                quoteCount++;
            }
        }

        // If odd number of quotes, we're inside a string
        return (quoteCount % 2) == 1;
    }

    std::string PathCompletionHandler::extractImportPath(const std::string& content, const Position& position)
    {
        std::istringstream stream(content);
        std::string line;
        int currentLine = 0;

        // Get the line at the cursor position
        while (std::getline(stream, line) && currentLine < position.line)
        {
            currentLine++;
        }

        if (currentLine != position.line)
        {
            return "";
        }

        // Find the opening quote before cursor
        int quoteStart = -1;
        for (int i = position.character - 1; i >= 0; i--)
        {
            if (line[i] == '"')
            {
                quoteStart = i + 1;
                break;
            }
        }

        if (quoteStart == -1)
        {
            return "";
        }

        // Extract path from opening quote to cursor
        return line.substr(quoteStart, position.character - quoteStart);
    }

    std::string PathCompletionHandler::resolveRelativePath(const std::string& baseUri, const std::string& relativePath)
    {
        std::string basePath = UriUtils::uriToFilePath(baseUri);

        try
        {
            fs::path currentFilePath(basePath);
            fs::path currentDir = currentFilePath.parent_path();
            fs::path targetPath = currentDir / relativePath;
            return targetPath.lexically_normal().string();
        }
        catch (...)
        {
            return "";
        }
    }

    std::vector<CompletionItem> PathCompletionHandler::listDirectoryContents(
        const std::string& directoryPath,
        bool includeFiles,
        bool includeFolders
    )
    {
        std::vector<CompletionItem> items;

        try
        {
            fs::path dirPath(directoryPath);

            // If path doesn't exist or is not a directory, return empty
            if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
            {
                return items;
            }

            // Iterate through directory contents
            for (const auto& entry : fs::directory_iterator(dirPath))
            {
                CompletionItem item;

                if (entry.is_directory() && includeFolders)
                {
                    item.label = entry.path().filename().string() + "/";
                    item.kind = static_cast<int>(CompletionItemKind::Folder);
                    item.detail = "folder";
                    item.insertText = entry.path().filename().string() + "/";
                    items.push_back(item);
                }
                else if (entry.is_regular_file() && includeFiles)
                {
                    std::string filename = entry.path().filename().string();

                    // Only show .mt files (C++17 compatible check)
                    if (filename.length() >= 3 &&
                        filename.substr(filename.length() - 3) == ".mt")
                    {
                        item.label = filename;
                        item.kind = static_cast<int>(CompletionItemKind::File);
                        item.detail = "mType file";
                        item.insertText = filename;
                        items.push_back(item);
                    }
                }
            }

            // Sort items: folders first, then files, alphabetically
            std::sort(items.begin(), items.end(), [](const CompletionItem& a, const CompletionItem& b)
            {
                // Folders (ending with /) come before files
                bool aIsFolder = !a.label.empty() && a.label.back() == '/';
                bool bIsFolder = !b.label.empty() && b.label.back() == '/';

                if (aIsFolder != bIsFolder)
                {
                    return aIsFolder; // Folders first
                }

                return a.label < b.label; // Alphabetical
            });
        }
        catch (const std::exception&)
        {
            // Return empty list on error
        }

        return items;
    }
} // namespace mtype::lsp
