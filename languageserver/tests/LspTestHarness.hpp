#pragma once

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mtype::lsp::test {

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct LspTestCase {
    std::string name;
    std::function<void()> fn;
};

class LspTestHarness {
public:
    explicit LspTestHarness(const std::string& suiteName)
        : suiteName_(suiteName) {}

    void addTest(const std::string& name, std::function<void()> fn) {
        tests_.push_back({name, std::move(fn)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n=== " << suiteName_ << " ===\n";

        for (const auto& test : tests_) {
            try {
                test.fn();
                std::cout << "  [PASS] " << test.name << "\n";
                ++passed;
            } catch (const std::exception& e) {
                std::cout << "  [FAIL] " << test.name << "\n"
                          << "         " << e.what() << "\n";
                ++failed;
            }
        }

        std::cout << "\n  " << passed << " passed, " << failed << " failed"
                  << " (" << tests_.size() << " total)\n";

        return failed;
    }

private:
    std::string suiteName_;
    std::vector<LspTestCase> tests_;
};

} // namespace mtype::lsp::test
