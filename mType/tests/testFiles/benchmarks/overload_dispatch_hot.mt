// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises overloaded method resolution and the generated call metadata.

class OverloadCalc {
    public function score(int a, int b): int {
        return a + b;
    }

    public function score(int a, int b, int c): int {
        return a + b + c;
    }

    public function score(float a, float b): float {
        return a + b;
    }

    public function score(string a, string b): string {
        return a + b;
    }
}

OverloadCalc calc = new OverloadCalc();
int N = 1000000;
int acc = 0;
float facc = 0.0;
int stringHits = 0;

for (int i = 0; i < N; i = i + 1) {
    acc = acc + calc.score(i, 3);
    acc = acc + calc.score(i, 2, 1);
    facc = facc + calc.score((float)i, 1.5);
    if (calc.score("a", "b") == "ab") {
        stringHits = stringHits + 1;
    }
}

print("overload_dispatch_hot acc=" + acc + " facc=" + facc + " strings=" + stringHits);
