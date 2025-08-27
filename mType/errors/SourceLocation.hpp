#pragma once
#include <string>
#include <sstream>

namespace errors
{
    class SourceLocation
    {
    private:
        std::string filename;
        int line;
        int column;

    public:
        explicit SourceLocation()
            : filename("<unknown>"), line(1), column(1)
        {
        }

        explicit SourceLocation(const std::string& file, int l, int c)
            : filename(file), line(l), column(c)
        {
        }

        std::string toString() const
        {
            std::stringstream ss;
            ss << filename << ":" << line << ":" << column;
            return ss.str();
        }
    };
}
