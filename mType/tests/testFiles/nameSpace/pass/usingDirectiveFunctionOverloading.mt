// Test using directive with basic function access
// This avoids potential overloading conflicts

namespace utils {
    function helper(int x): int {
        return x + 100;
    }
    
    int value = 42;
}

namespace tools {
    function assist(int x): int {
        return x * 3;
    }
    
    int counter = 99;
}

// Test 1: Using one namespace at a time
using namespace utils;

int result1 = helper(5);  // Should work fine
print(result1);  // Expected: 105

int val1 = value;  // Should work fine  
print(val1);  // Expected: 42

// Test 2: Qualified access to other namespace
int result2 = tools::assist(4);  // Qualified access
print(result2);  // Expected: 12

int val2 = tools::counter;  // Qualified access
print(val2);  // Expected: 99

// Test 3: Demonstrate both namespaces work with qualified access
int result3 = utils::helper(10);  // Explicit namespace access  
print(result3);  // Expected: 110

int result4 = tools::assist(7);  // Explicit namespace access
print(result4);  // Expected: 21