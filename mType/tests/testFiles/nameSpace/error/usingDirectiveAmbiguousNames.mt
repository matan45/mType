// Test error: undefined variable access after using directive

namespace math {
    function calculate(int x): int {
        return x * 2;
    }
}

using namespace math;

// This should cause an error - undefined variable
int result = undefinedVariable;  // ERROR: undefined variable 'undefinedVariable'