namespace math {
    final int pi = 3;
    final string name = "Mathematics";
    
    function getPi(): int {
        return pi; // Valid: reading final variable
    }
    
    function getName(): string {
        return name;
    }
}

// Valid operations: reading final variables
int piValue = math::getPi();
print(piValue);

namespace constants {
    final bool debug = true;
    final int maxValue = 1000;
}

// Test reading final variables from namespace
print(constants::debug);
print(constants::maxValue);