// Test class adds more overloads than interface requires
// @Script

interface Calculator {
    func add(a: Int, b: Int): Int;
}

class AdvancedCalculator implements Calculator {
    // Required by interface
    func add(a: Int, b: Int): Int {
        return a + b;
    }

    // Additional overload not in interface
    func add(a: Int, b: Int, c: Int): Int {
        return a + b + c;
    }

    // Another additional overload
    func add(numbers: Array<Int>): Int {
        var sum = 0;
        var i = 0;
        while (i < numbers.size()) {
            sum = sum + numbers.get(i);
            i = i + 1;
        }
        return sum;
    }
}

var calc = new AdvancedCalculator();

print(calc.add(5, 10));           // Should print 15
print(calc.add(5, 10, 15));       // Should print 30

var nums = new Array<Int>();
nums.add(1);
nums.add(2);
nums.add(3);
nums.add(4);
print(calc.add(nums));            // Should print 10
