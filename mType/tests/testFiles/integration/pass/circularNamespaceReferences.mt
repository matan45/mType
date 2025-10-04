// Test potential circular reference issues

    final int VALUE_A = 100;
    
    class ComponentA {
        public int data;

        public constructor() {
            data = VALUE_A;
        }

        public function processWithB(): int {
            // This should work - forward reference
            return ComponentB::processData(data);
        }
    }



    final int VALUE_B = 200;
    
    class ComponentB {

        public constructor() {}

        public static function processData(int input): int {
            return input + VALUE_B;
        }

        public function processWithA(): int {
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