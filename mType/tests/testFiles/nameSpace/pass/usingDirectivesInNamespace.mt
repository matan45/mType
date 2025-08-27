namespace math {
    function multiply(int a, int b): int {
        return a * b;
    }
    
    function add(int a, int b): int {
        return a + b;
    }
}

namespace calculator {
    using namespace math;
    
    function calculate(int x, int y): int {
        int product = multiply(x, y);  // Using math::multiply via using directive
        int sum = add(x, y);           // Using math::add via using directive
        return product + sum;
    }
}

namespace advanced {
    namespace operations {
        function square(int x): int {
            return x * x;
        }
    }
}

namespace utilities {
    using namespace advanced::operations;
    
    function processValue(int n): int {
        return square(n);  // Using advanced::operations::square via using directive
    }
}

// Test using directives inside namespaces
int result1 = calculator::calculate(3, 4);  // Should use math functions via using
print(result1);

int result2 = utilities::processValue(5);   // Should use advanced::operations::square via using
print(result2);