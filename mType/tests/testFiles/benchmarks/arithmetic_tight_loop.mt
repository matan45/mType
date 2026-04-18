// TARGET: ~2s on dev machine. Adjust N / M if first run lands outside 1-5s.
// Exercises integer arithmetic in a tight loop plus a smaller float loop.

int intSum = 0;
int N = 10000000;
for (int i = 0; i < N; i = i + 1) {
    intSum = intSum + i * i - i;
}

float floatSum = 0.0;
int M = 2000000;
for (int i = 0; i < M; i = i + 1) {
    floatSum = floatSum + (i + 0.5);
}

print("arithmetic_tight_loop intSum=" + intSum);
print(floatSum);
