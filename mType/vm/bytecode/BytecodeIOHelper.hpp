#pragma once
#include <ostream>
#include <istream>
#include <string>
#include <vector>

namespace vm::bytecode
{
    /**
     * Utility class for bytecode serialization I/O operations
     * Eliminates duplication in reading/writing strings and primitive types
     */
    class BytecodeIOHelper
    {
    public:
        // String I/O
        static void writeString(std::ostream& out, const std::string& str);
        static std::string readString(std::istream& in);

        // String vector I/O
        static void writeStringVector(std::ostream& out, const std::vector<std::string>& vec);
        static std::vector<std::string> readStringVector(std::istream& in);

        // Template for primitive types
        template<typename T>
        static void writePrimitive(std::ostream& out, const T& value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        template<typename T>
        static T readPrimitive(std::istream& in) {
            T value;
            in.read(reinterpret_cast<char*>(&value), sizeof(value));
            return value;
        }
    };
}
