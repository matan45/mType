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
        SourceLocation()
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

        // Getters for serialization
        const std::string& getFilename() const { return filename; }
        int getLine() const { return line; }
        int getColumn() const { return column; }

        // Setters for deserialization
        void setFilename(const std::string& file) { filename = file; }
        void setLine(int l) { line = l; }
        void setColumn(int c) { column = c; }
    };
}
