// Test error: static modifier on using directive (invalid syntax)

namespace utils {
    function process(int x): int {
        return x + 10;
    }
}

// Static modifier cannot be applied to using directives
static using namespace utils;  // ERROR: static modifier not allowed on using directive

// This code should never be reached due to parse error
print("This should not print");