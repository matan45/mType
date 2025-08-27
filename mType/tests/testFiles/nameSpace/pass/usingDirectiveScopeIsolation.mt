// Test using directive scope isolation - using directives should only affect their scope

namespace tools {
    function helper(int x): int {
        return x + 50;
    }
    
    int toolValue = 500;
}

namespace section1 {
    using namespace tools;
    
    function testInSection1(): int {
        // tools namespace should be available here
        int result = helper(10);  // Should work - tools::helper
        return result + toolValue;  // Should work - tools::toolValue
    }
}

namespace section2 {
    // No using directive here - tools should not be directly available
    
    function testInSection2(): int {
        // This should require qualified access since no using directive
        int result = tools::helper(20);  // Must use qualified name
        return result + tools::toolValue;  // Must use qualified name
    }
}

// Test 1: Section1 should work with using directive
int result1 = section1::testInSection1();
print(result1);  // Expected: 560 (10+50+500)

// Test 2: Section2 should work with qualified access
int result2 = section2::testInSection2();
print(result2);  // Expected: 570 (20+50+500)

// Test 3: Global scope should not have access without qualified names
int globalResult = tools::helper(5);  // Must use qualified name
print(globalResult);  // Expected: 55