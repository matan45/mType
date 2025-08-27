// Test that namespaces don't pollute the global scope
// Variables and functions defined in namespaces should not be accessible globally without qualification

namespace utils {
    int helperValue = 42;
    string secretMessage = "hidden";
    bool isActive = true;
    
    function helper(int x): int {
        return x * 2;
    }
    
    function processData(string data): string {
        return "processed: " + data;
    }
}

namespace math {
    int constant = 100;
    
    function calculate(int a, int b): int {
        return a + b + constant;
    }
}

// Test 1: Namespace variables should be accessible with qualification
int value1 = utils::helperValue;
print(value1);  // Expected: 42

string msg = utils::secretMessage;
print(msg);  // Expected: hidden

bool active = utils::isActive;
print(active);  // Expected: true

// Test 2: Namespace functions should work with qualification
int result1 = utils::helper(5);
print(result1);  // Expected: 10

string processed = utils::processData("test");
print(processed);  // Expected: processed: test

int calc = math::calculate(10, 20);
print(calc);  // Expected: 130

// Test 3: Define global variables with same names - should not conflict
int helperValue = 999;  // Different from utils::helperValue
string secretMessage = "global";  // Different from utils::secretMessage

print(helperValue);    // Expected: 999 (global, not utils::helperValue)
print(secretMessage);  // Expected: global (global, not utils::secretMessage)

// Test 4: Verify namespace values are still accessible and unchanged
print(utils::helperValue);    // Expected: 42 (unchanged)
print(utils::secretMessage);  // Expected: hidden (unchanged)

// Test 5: Global functions with same names should not conflict with namespace functions
function helper(int x): int {
    return x + 1000;  // Different behavior from utils::helper
}

function calculate(int a, int b): int {
    return a * b;  // Different behavior from math::calculate
}

// Test global functions work
int globalHelper = helper(5);
print(globalHelper);  // Expected: 1005

int globalCalc = calculate(10, 20);
print(globalCalc);  // Expected: 200

// Test namespace functions still work unchanged
int nsHelper = utils::helper(5);
print(nsHelper);  // Expected: 10

int nsCalc = math::calculate(10, 20);
print(nsCalc);  // Expected: 130

// Test passed if all outputs match expected values
print("Namespace isolation test passed");