#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace mtype::lsp::test {

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() {
        static int counter = 0;
        path_ = fs::temp_directory_path() / ("mtype_test_" + std::to_string(++counter)
                + "_" + std::to_string(std::time(nullptr)));
        fs::create_directories(path_);
    }

    ~TempDir() {
        if (fs::exists(path_)) {
            fs::remove_all(path_);
        }
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::string path() const { return path_.string(); }

    void createFile(const std::string& relativePath, const std::string& content) {
        fs::path filePath = path_ / relativePath;
        fs::create_directories(filePath.parent_path());
        std::ofstream out(filePath);
        out << content;
    }

private:
    fs::path path_;
};

} // namespace mtype::lsp::test
