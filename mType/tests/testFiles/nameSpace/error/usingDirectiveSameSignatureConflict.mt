// Test error: ambiguous function reference with using directives

namespace math {
    function calculate(int x): int {
        return x * 2;
    }
}

namespace math2 {
    function calculate(int x): int {
        return x * 2;
    }
}

using namespace math;
using namespace math2;

// This should cause an ambiguity error - multiple matching functions
int result = calculate(5);  // ERROR: Ambiguous function reference