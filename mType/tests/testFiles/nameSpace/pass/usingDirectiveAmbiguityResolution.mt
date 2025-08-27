// Test ambiguous name resolution with using directives
// This should demonstrate how the language resolves ambiguity

namespace math {
    function calculate(int x): int {
        return x * 2;
    }
    
    int value = 100;
}

namespace physics {
    function calculate(int x): int {
        return x * 3;
    }
    
    int value = 200;
}

// Test 1: Only one using directive - should work fine
using namespace math;
int result1 = calculate(5);  // Should use math::calculate
print(result1);  // Expected: 10

int val1 = value;  // Should use math::value
print(val1);  // Expected: 100

// Test 2: Qualified access still works even with using directive
int result2 = physics::calculate(5);  // Should use physics::calculate explicitly
print(result2);  // Expected: 15

int val2 = physics::value;  // Should use physics::value explicitly
print(val2);  // Expected: 200