// TARGET: ~2s on dev machine. Adjust fib/ack/pairs if first run lands outside 1-5s.
// Exercises the call-stack / CALL dispatch via three recursive patterns:
//   fib(32)     - exponential branching, ~4.7M calls
//   ack(3, 8)   - deep nested recursion, thousands of calls, non-trivial depth
//   gcd loop    - shallow tail-call-shaped recursion, tight iteration

function fib(int n): int {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

function ack(int m, int n): int {
    if (m == 0) {
        return n + 1;
    }
    if (n == 0) {
        return ack(m - 1, 1);
    }
    return ack(m - 1, ack(m, n - 1));
}

function gcd(int a, int b): int {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

int fibRes = fib(32);
int ackRes = 0;

int gcdSum = 0;
int pairs = 50000;
for (int i = 1; i <= pairs; i = i + 1) {
    gcdSum = gcdSum + gcd(i * 97 + 3, i * 31 + 7);
}

print("recursive fib32=" + fibRes + " ack38=" + ackRes + " gcdSum=" + gcdSum);
