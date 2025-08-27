// Test using directive shadowing behavior
// Local declarations should shadow using directive imports

namespace utils {
    function process(int x): int {
        return x + 10;
    }
    
    int counter = 50;
}

using namespace utils;

// Test 1: Local function shadows using directive import
function process(int x): int {
    return x * 5;  // Local version - different behavior
}

int result1 = process(6);  // Should use local process function
print(result1);  // Expected: 30 (6 * 5, not 6 + 10)

// Test 2: Local variable shadows using directive import
int counter = 25;  // Local counter shadows utils::counter

print(counter);  // Should use local counter
// Expected: 25 (not 50)

// Test 3: Qualified access still works for shadowed names
int result2 = utils::process(6);  // Explicitly use utils version
print(result2);  // Expected: 16 (6 + 10)

int utils_counter = utils::counter;  // Explicitly use utils version
print(utils_counter);  // Expected: 50