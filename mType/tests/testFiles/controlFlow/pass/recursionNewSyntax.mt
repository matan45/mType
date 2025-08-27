function gcd(int a, int b): int {
   if (b == 0) {
        return a; // base case
    }
    return gcd(b, a % b);
}

function power(int base, int exp): int {
    if (exp == 0) {
        return 1;
    }
    if (exp == 1) {
        return base;
    }
    
    return base * power(base, exp - 1);
}

function sumDigits(int n): int {
    if (n < 10) {
        return n;
    }
    return (n % 10) + sumDigits(n / 10);
}

print(gcd(48, 18));      // 6
print(power(2, 10));     // 1024
print(sumDigits(12345)); // 15
print("Test passed"); // Test completed