// Test: JIT null check elimination for non-nullable receivers
// Functions called 200+ times to trigger JIT compilation (threshold=100)
// Non-nullable params should have null checks eliminated in JIT

class Counter {
    public int count;
    constructor(int c) {
        this.count = c;
    }
    public function increment(): void {
        this.count = this.count + 1;
    }
    public function toString(): string {
        return "count=" + this.count;
    }
}

// Hot function: pure int arithmetic (JIT-safe)
function addValues(int a, int b): int {
    return a + b;
}

// Hot function: non-nullable field set (null check eliminated)
function incrementCounter(Counter c): void {
    c.count = c.count + 1;
}

// Hot function: nullable-safe path returning string (BOXED, no typed arithmetic)
function safeDescribe(Counter? c): string {
    if (c != null) {
        return c.toString();
    }
    return "null";
}

// === Test 1: Hot int arithmetic ===
int sum = 0;
for (int i = 0; i < 200; i = i + 1) {
    sum = addValues(sum, 1);
}
print("sum: " + sum);

// === Test 2: Hot non-nullable field access (null check eliminated) ===
Counter counter = new Counter(0);
for (int i = 0; i < 200; i = i + 1) {
    incrementCounter(counter);
}
print("counter: " + counter.count);

// === Test 3: Hot nullable path (null check preserved) ===
Counter? maybeCounter = new Counter(100);
string lastResult = "";
for (int i = 0; i < 200; i = i + 1) {
    if (i % 2 == 0) {
        lastResult = safeDescribe(maybeCounter);
    } else {
        lastResult = safeDescribe(null);
    }
}
// Last iteration i=199 is odd
print("last result: " + lastResult);

// === Test 4: Hot method calls on non-nullable ===
Counter c2 = new Counter(0);
for (int i = 0; i < 200; i = i + 1) {
    c2.increment();
}
print("c2: " + c2.count);

print("JIT null check elimination tests passed!");
