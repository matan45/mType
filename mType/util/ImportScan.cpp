#include "ImportScan.hpp"

#include <regex>
#include <sstream>

namespace util
{
    int findImportInsertLine(const std::string& sourceContent)
    {
        // static const so the DFA is compiled once at first call, not on
        // every invocation. std::regex construction is moderately
        // expensive and this function is called per code-action and per
        // auto-import completion request.
        static const std::regex importRegex("^\\s*import\\b");

        int insertLine = 0;
        std::istringstream stream(sourceContent);
        std::string current;
        int lineNo = 0;
        while (std::getline(stream, current) && lineNo < 50)
        {
            if (std::regex_search(current, importRegex))
            {
                insertLine = lineNo + 1; // insert after the matched line
            }
            ++lineNo;
        }
        return insertLine;
    }
}
