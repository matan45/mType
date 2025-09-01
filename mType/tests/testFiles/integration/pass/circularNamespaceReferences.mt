// Test potential circular reference issues

    final int VALUE_A = 100;
    
    class ComponentA {
        int data;
        
        constructor() {
            data = VALUE_A;
        }
        
        function processWithB(): int {
            // This should work - forward reference
            return ComponentB::processData(data);
        }
    }



    final int VALUE_B = 200;
    
    class ComponentB {
        
        constructor() {}

        static function processData(int input): int {
            return input + VALUE_B;
        }
        
        function processWithA(): int {
            ComponentA compA = new ComponentA();
            return compA.data + VALUE_B;
        }
    }


// Test circular-like interactions
ComponentA compA = new ComponentA();
ComponentB compB = new ComponentB();

int result1 = compA.processWithB();
int result2 = compB.processWithA();
print(result1);
print(result2);