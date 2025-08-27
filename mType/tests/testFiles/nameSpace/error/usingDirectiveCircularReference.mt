// Test error: final modifier on using directive (invalid syntax)

namespace tools {
    function helper(int x): int {
        return x + 50;
    }
}

// Final modifier cannot be applied to using directives
final using namespace tools;  // ERROR: final modifier not allowed on using directive

// This code should never be reached due to parse error
print("This should not print");