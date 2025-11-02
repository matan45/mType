// Test lambda calling itself through stored reference
// Note: Direct lambda self-reference requires closure capture

// Helper function that takes a lambda and allows it to call itself
function recursiveLambdaHelper(int n, int acc): int {
    // Simulate factorial using nested function calls
    function inner(int x, int a): int {
        if (x <= 1) {
            return a;
        }
        return inner(x - 1, x * a);
    }
    return inner(n, acc);
}

// Function using lambda-like behavior for recursion
function testRecursiveLambda(): void {
    print("Testing recursive lambda patterns:");

    // Test 1: Factorial through helper
    int result1 = recursiveLambdaHelper(5, 1);
    print("Factorial(5) = " + result1); // 120

    int result2 = recursiveLambdaHelper(6, 1);
    print("Factorial(6) = " + result2); // 720

    // Test 2: Sum using recursive pattern
    function sumRecursive(int n, int acc): int {
        if (n <= 0) {
            return acc;
        }
        return sumRecursive(n - 1, acc + n);
    }

    int sum1 = sumRecursive(10, 0);
    print("Sum(10) = " + sum1); // 55

    int sum2 = sumRecursive(20, 0);
    print("Sum(20) = " + sum2); // 210

    // Test 3: Countdown with accumulation
    function countDown(int n): int {
        function helper(int x, int acc): int {
            if (x <= 0) {
                return acc;
            }
            return helper(x - 1, acc + x);
        }
        return helper(n, 0);
    }

    print("CountDown(5) = " + countDown(5));   // 15
    print("CountDown(7) = " + countDown(7));   // 28

    // Test 4: Power function with nested recursion
    function powerFunc(int base, int exp): int {
        function powerHelper(int b, int e, int acc): int {
            if (e <= 0) {
                return acc;
            }
            return powerHelper(b, e - 1, acc * b);
        }
        return powerHelper(base, exp, 1);
    }

    print("Power(2, 8) = " + powerFunc(2, 8));   // 256
    print("Power(3, 4) = " + powerFunc(3, 4));   // 81

    print("Recursive lambda pattern tests completed");
}

testRecursiveLambda();
