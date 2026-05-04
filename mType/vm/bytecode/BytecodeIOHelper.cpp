#include "BytecodeIOHelper.hpp"
#include <cstddef>
#include <istream>
#include <ostream>

namespace vm::bytecode
{
    void BytecodeIOHelper::writeString(std::ostream& out, const std::string& str) {
        size_t len = str.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));

        if (!out) {
            throw std::runtime_error("Failed to write string length to bytecode stream");
        }

        if (len > 0) {
            out.write(str.data(), len);

            if (!out) {
                throw std::runtime_error("Failed to write string data to bytecode stream");
            }
        }
    }

    std::string BytecodeIOHelper::readString(std::istream& in) {
        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));

        if (!in) {
            throw std::runtime_error("Failed to read string length from bytecode stream");
        }

        // Prevent memory exhaustion attacks from malicious bytecode files
        if (len > MAX_STRING_LENGTH) {
            throw std::runtime_error(
                "String length " + std::to_string(len) +
                " exceeds maximum allowed size of " + std::to_string(MAX_STRING_LENGTH) + " bytes"
            );
        }

        std::string result;
        if (len > 0) {
            result.resize(len);
            in.read(&result[0], len);

            if (!in) {
                throw std::runtime_error("Failed to read string data from bytecode stream");
            }
        }
        return result;
    }

    void BytecodeIOHelper::writeStringVector(std::ostream& out, const std::vector<std::string>& vec) {
        size_t count = vec.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        if (!out) {
            throw std::runtime_error("Failed to write vector count to bytecode stream");
        }

        for (const auto& str : vec) {
            writeString(out, str);  // Will throw if write fails
        }
    }

    std::vector<std::string> BytecodeIOHelper::readStringVector(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        if (!in) {
            throw std::runtime_error("Failed to read vector count from bytecode stream");
        }

        // Prevent excessive allocation from malicious bytecode files
        if (count > MAX_VECTOR_SIZE) {
            throw std::runtime_error(
                "Vector count " + std::to_string(count) +
                " exceeds maximum allowed size of " + std::to_string(MAX_VECTOR_SIZE) + " elements"
            );
        }

        std::vector<std::string> result;
        result.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            result.push_back(readString(in));  // Will throw if individual string is too large
        }

        return result;
    }
}
