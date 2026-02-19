// JIT Test: Object creation, method calls, static calls, instanceof/cast

class Counter {
    public int count;

    public constructor(int initial) {
        this.count = initial;
    }

    public function increment(): void {
        this.count = this.count + 1;
    }

    public function add(int n): void {
        this.count = this.count + n;
    }

    public function getCount(): int {
        return this.count;
    }
}

class MathHelper {
    public static function square(int x): int {
        return x * x;
    }

    public static function clamp(int val, int lo, int hi): int {
        if (val < lo) {
            return lo;
        }
        if (val > hi) {
            return hi;
        }
        return val;
    }

    public static function sumSquares(int n): int {
        int total = 0;
        for (int i = 0; i < n; i = i + 1) {
            total = total + i * i;
        }
        return total;
    }
}

class Animal {
    public string name;

    public constructor(string n) {
        this.name = n;
    }

    public function speak(): string {
        return this.name + " makes a sound";
    }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {
    }

	@Override
    public function speak(): string {
        return this.name + " barks";
    }
}

class Cat extends Animal {
    public constructor(string n) : super(n) {
    }

	@Override
    public function speak(): string {
        return this.name + " meows";
    }
}

// Heavy function to JIT-compile via regular CALL path
function accumulate(int n): int {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + i;
    }
    return total;
}

function clampLoop(int iterations): int {
    int sum = 0;
    for (int i = 0; i < iterations; i = i + 1) {
        if (i < 10) {
            sum = sum + 10;
        } else if (i > 500) {
            sum = sum + 500;
        } else {
            sum = sum + i;
        }
    }
    return sum;
}

// Warm up JIT with hot functions
for (int i = 0; i < 150; i = i + 1) {
    accumulate(10);
    clampLoop(10);
}

// Heavy accumulation (JIT-compiled function called many times)
int accResult = 0;
for (int i = 0; i < 10000; i = i + 1) {
    accResult = accResult + accumulate(100);
}
print("10000x accumulate(100): " + accResult);

// Heavy clamping loop
int clampResult = 0;
for (int i = 0; i < 5000; i = i + 1) {
    clampResult = clampResult + clampLoop(1000);
}
print("5000x clampLoop(1000): " + clampResult);

// Static method calls
print("sumSquares(1000): " + MathHelper::sumSquares(1000));

// Test object method calls
Counter counter = new Counter(0);
for (int i = 0; i < 10000; i = i + 1) {
    counter.add(i);
}
print("counter after 10000 adds: " + counter.getCount());

// Test instanceof and polymorphism
Animal dog = new Dog("Rex");
Animal cat = new Cat("Whiskers");
print(dog.speak());
print(cat.speak());
print("dog isClassOf Dog: " + (dog isClassOf Dog));
print("dog isClassOf Cat: " + (dog isClassOf Cat));
print("dog isClassOf Animal: " + (dog isClassOf Animal));

// Test cast
if (dog isClassOf Dog) {
    Dog d = (Dog)dog;
    print("cast ok: " + d.speak());
}
