// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises reflection metadata lookup without invoking methods.

import * from "../lib/reflect/Class.mt";
import * from "../lib/reflect/Field.mt";
import * from "../lib/reflect/Method.mt";

class ReflectBenchTarget {
    public int value;
    public static int count = 0;

    public constructor(int value) {
        this.value = value;
    }

    public function add(int x): int {
        return this.value + x;
    }

    public function mul(int x): int {
        return this.value * x;
    }

    public static function staticAdd(int a, int b): int {
        return a + b;
    }
}

Class klass = Class::forName("ReflectBenchTarget");

int N = 200000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    Field field = klass.getDeclaredField("value");
    Method add = klass.getMethod("add", 1);
    Method mul = klass.getMethod("mul", 1);
    total = total + strLength(field.getName()) + add.getParameterCount() + mul.getParameterCount();
}

print("reflection_lookup_hot total=" + total);
