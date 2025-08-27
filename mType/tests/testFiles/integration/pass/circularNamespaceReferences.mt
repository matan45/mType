// Test potential circular reference issues
namespace a {
    final int VALUE_A = 100;
    
    class ComponentA {
        int data;
        
        constructor() {
            data = VALUE_A;
        }
        
        function processWithB(): int {
            // This should work - forward reference
            return b::ComponentB::processData(data);
        }
    }
}

namespace b {
    final int VALUE_B = 200;
    
    class ComponentB {
        
        constructor() {}

        static function processData(int input): int {
            return input + VALUE_B;
        }
        
        function processWithA(): int {
            a::ComponentA compA = new a::ComponentA();
            return compA.data + VALUE_B;
        }
    }
}

// Test circular-like interactions
a::ComponentA compA = new a::ComponentA();
b::ComponentB compB = new b::ComponentB();

int result1 = compA.processWithB();
int result2 = compB.processWithA();
print(result1);
print(result2);