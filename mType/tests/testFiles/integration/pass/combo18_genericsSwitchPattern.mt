// Combo 18: Generic Box<T> + Pattern Matching
// Tests: three Box<T> instantiations dispatched through match on T

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";

class Box<T> {
    private T value;
    private string label;

    public constructor(T value, string label) {
        this.value = value;
        this.label = label;
    }

    public function getValue(): T { return this.value; }
    public function getLabel(): string { return this.label; }
}

function describe(Object boxedValue): string {
    string result = "";
    match (boxedValue) {
        case Int i -> {
            result = "int=" + i.getValue();
        }
        case String s -> {
            result = "str=" + s.getValue();
        }
        case Float f -> {
            result = "float=" + f.getValue();
        }
        default -> {
            result = "unknown";
        }
    }
    return result;
}

function main(): void {
    print("=== Combo 18: Generics + Match Pattern ===");

    Box<Int> bi = new Box<Int>(new Int(42), "answer");
    Box<String> bs = new Box<String>(new String("hello"), "greeting");
    Box<Float> bf = new Box<Float>(new Float(3.14), "pi");

    print("--- Three boxes ---");
    print(bi.getLabel() + " -> " + describe(bi.getValue()));
    print(bs.getLabel() + " -> " + describe(bs.getValue()));
    print(bf.getLabel() + " -> " + describe(bf.getValue()));

    print("--- Direct dispatch ---");
    print(describe(new Int(7)));
    print(describe(new String("world")));
    print(describe(new Float(2.5)));

    print("=== Combo 18 Complete ===");
}

main();
