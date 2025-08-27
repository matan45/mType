// Test using directives in nested scopes and potential conflicts

namespace outer {
    function transform(int x): int {
        return x + 100;
    }
    
    int baseValue = 1000;
}

namespace inner {
    function transform(int x): int {
        return x * 10;
    }
    
    int baseValue = 2000;
}


// Test 1: Global using directive works
int result1 = outer::transform(5);  // Should use outer::transform
print(result1);  // Expected: 105

int val1 = outer::baseValue;  // Should use outer::baseValue
print(val1);  // Expected: 1000

namespace test {
    // Nested namespace with its own using directive
    using namespace inner;
    
    function runTest(): int {
        // This should use inner::transform due to local using directive
        int result = transform(7);  // Should use inner::transform
        return result + baseValue;  // Should use inner::baseValue
    }
}

// Test 2: Nested scope using directive takes precedence
int result2 = test::runTest();
print(result2);  // Expected: 2070 (7*10 + 2000)

using namespace outer;  // Global using directive

// Test 3: Outside the nested scope, global using directive still works
int result3 = transform(3);  // Should still use outer::transform
print(result3);  // Expected: 103