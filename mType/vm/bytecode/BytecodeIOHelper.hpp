#pragma once
#include <ostream>
#include <cstddef>
#include <istream>
#include <string>
#include <vector>
#include <stdexcept>

namespace vm::bytecode
{
    /**
     * Utility class for bytecode serialization I/O operations
     * Eliminates duplication in reading/writing strings and primitive types
     * Includes security measures to prevent memory exhaustion attacks
     */
    class BytecodeIOHelper
    {
    public:
        // Security limits for bytecode deserialization
        // These prevent memory exhaustion attacks from malicious bytecode files
        static constexpr size_t MAX_STRING_LENGTH = 1024 * 1024 * 1024;  // 1 GB per string
        static constexpr size_t MAX_VECTOR_SIZE = 10'000'000;            // 10M elements per vector

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

            if (!out) {
                throw std::runtime_error("Failed to write primitive value to bytecode stream");
            }
        }

        template<typename T>
        static T readPrimitive(std::istream& in) {
            T value;
            in.read(reinterpret_cast<char*>(&value), sizeof(value));

            if (!in) {
                throw std::runtime_error("Failed to read primitive value from bytecode stream");
            }

            return value;
        }
    };
}
