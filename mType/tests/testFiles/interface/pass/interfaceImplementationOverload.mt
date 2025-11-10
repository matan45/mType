// Test class adds more overloads than interface requires
// @Script

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";

interface Calculator {
    function add(int a, int b): int;
}

class AdvancedCalculator implements Calculator {
    // Required by interface
    public function add(int a, int b): int {
        return a + b;
    }

    // Additional overload not in interface
    public function add2(int a, int b, int c): int {
        return a + b + c;
    }

    // Another additional overload
    public function addList(List<Int> numbers): int {
        int sum = 0;
        for (int i = 0; i < numbers.size(); i++) {
            sum = sum + numbers.get(i).getValue();
        }
        return sum;
    }
}

AdvancedCalculator calc = new AdvancedCalculator();

print(calc.add(5, 10));           // Should print 15
print(calc.add2(5, 10, 15));       // Should print 30

List<Int> nums = new List<Int>();
nums.add(new Int(1));
nums.add(new Int(2));
nums.add(new Int(3));
nums.add(new Int(4));
print(calc.addList(nums));            // Should print 10
