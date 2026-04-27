// Edge Case: Overloaded methods with generic + nullable parameters

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Resolver {
    // Overload: specific int
    public function resolve(int value): string {
        return "int: " + value;
    }

    // Overload: specific string
    public function resolve(string value): string {
        return "string: " + value;
    }

    // Nullable boxed String
    public function resolveNullable(String? value): string {
        if (value == null) {
            return "null";
        }
        return "nullable-string: " + value.getValue();
    }

    // Nullable boxed Int
    public function resolveNullable(Int? value): string {
        if (value == null) {
            return "null-int";
        }
        return "nullable-int: " + value.getValue();
    }

    // Generic with nullable
    public function <T> resolveGenericNullable(T? value, string fallback): string {
        if (value == null) {
            return "fallback: " + fallback;
        }
        return "value: " + value;
    }
}

function main(): void {
    print("=== Edge: Overload Generic + Nullable ===");

    Resolver r = new Resolver();

    // Specific overloads
    print("--- Specific overloads ---");
    print(r.resolve(42));
    print(r.resolve("hello"));

    // Nullable overloads (using boxed types)
    print("--- Nullable overloads ---");
    print(r.resolveNullable(new String("test")));
    String? nullStr = null;
    print(r.resolveNullable(nullStr));
    print(r.resolveNullable(new Int(99)));
    Int? nullInt = null;
    print(r.resolveNullable(nullInt));

    // Generic nullable
    print("--- Generic nullable ---");
    print(r.resolveGenericNullable(new String("present"), "default"));
    String? absent = null;
    print(r.resolveGenericNullable(absent, "default"));

    // Box type overloads
    print("--- Box type resolve ---");
    Int boxedInt = new Int(100);
    print(r.resolve(boxedInt.getValue()));

    String boxedStr = new String("boxed");
    print(r.resolve(boxedStr.getValue()));

    print("=== Edge Complete ===");
}

main();
