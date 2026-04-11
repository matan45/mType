#include "ImportScan.hpp"

#include <regex>
#include <sstream>

namespace util
{
    int findImportInsertLine(const std::string& sourceContent)
    {
        int insertLine = 0;
        std::istringstream stream(sourceContent);
        std::string current;
        int lineNo = 0;
        std::regex importRegex("^\\s*import\\b");
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
