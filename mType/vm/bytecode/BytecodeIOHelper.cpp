#include "BytecodeIOHelper.hpp"
#include <istream>
#include <ostream>

namespace vm::bytecode
{
    void BytecodeIOHelper::writeString(std::ostream& out, const std::string& str) {
        size_t len = str.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) {
            out.write(str.data(), len);
        }
    }

    std::string BytecodeIOHelper::readString(std::istream& in) {
        size_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));

        std::string result;
        if (len > 0) {
            result.resize(len);
            in.read(&result[0], len);
        }
        return result;
    }

    void BytecodeIOHelper::writeStringVector(std::ostream& out, const std::vector<std::string>& vec) {
        size_t count = vec.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& str : vec) {
            writeString(out, str);
        }
    }

    std::vector<std::string> BytecodeIOHelper::readStringVector(std::istream& in) {
        size_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        std::vector<std::string> result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            result.push_back(readString(in));
        }
        return result;
    }
}
