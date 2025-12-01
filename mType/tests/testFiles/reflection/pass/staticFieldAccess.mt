// Test static field access via reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class Counter {
    public static int count = 0;
    public static int max = 100;

    public constructor() {
        count = count + 1;
    }

    public static function reset(): void {
        count = 0;
    }
}

// Create some instances to change count
Counter c1 = new Counter();
Counter c2 = new Counter();
Counter c3 = new Counter();

print("Direct access - count: " + Counter::count);

// Access via reflection
Class counterClass = Class::forName("Counter");

Field countField = counterClass.getDeclaredField("count");
print("countField is static: " + countField.isStatic());

// Get static field value (pass 0 for static fields)
int countValue = countField.getInt(0);
print("Reflection access - count: " + countValue);

Field maxField = counterClass.getDeclaredField("max");
int maxValue = maxField.getInt(0);
print("Reflection access - max: " + maxValue);

print("Test passed");
