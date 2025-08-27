// Function with final parameters would require more changes
// For now, test final variables inside functions

function calculateArea(float radius): float {
    final float PI = 3.14159;
    // PI = 3.14; // This would be an error
    return PI * radius * radius;
}

function getConfig(): int {
    final int CONFIG_VALUE = 42;
    final int MULTIPLIER = 2;
    
    // Can use final variables
    int result = CONFIG_VALUE * MULTIPLIER;
    
    return result;
}

float area = calculateArea(5.0);
print(area);  // Should print ~78.54

int config = getConfig();
print(config);  // Should print 84

// Final global constant
final int GLOBAL_MAX = 1000;

function useGlobal(): void {
    print(GLOBAL_MAX);  // Can read final variable
    // GLOBAL_MAX = 2000; // This would be an error
}

useGlobal();  // Should print 1000
print("Test passed"); // Test completed