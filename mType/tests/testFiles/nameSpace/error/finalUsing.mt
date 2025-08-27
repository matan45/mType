namespace utils {
    function max(int a, int b): int {
        if (a > b) {
            return a;
        }
        return b;
    }
}

// Final modifier cannot be applied to using directives
final using namespace utils;

// This code should never be reached due to parse error
print("This should not print");