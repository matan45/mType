function factorial(int n): int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

function fibonacci(int n): int {
    if (n <= 1) {
        return n;
    }
    
    int prev = 0;
    int curr = 1;
    
    for (int i = 2; i <= n; i++) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    
    return curr;
}

function isPrime(int n): bool {
    if (n <= 1) {
        return false;
    }
    if (n <= 3) {
        return true;
    }
    if (n - (n / 2) * 2 == 0) {  // Check if even
        return false;
    }
    
    for (int i = 3; i * i <= n; i = i + 2) {
        if (n - (n / i) * i == 0) {
            return false;
        }
    }
    return true;
}

print(factorial(5));   // 120
print(fibonacci(10));  // 55
print(isPrime(17));    // 1 (true)
print(isPrime(18));    // 0 (false)
print("Test passed"); // Test completed