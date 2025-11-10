import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type inference failure - conflicting types
class Box<T> {
    T value;

    public function setValue(T val): void {
        value = val;
    }
}

function <T> process(T a, T b): Box<T> {
    Box<T> box = new Box<T>();
    box.setValue(a);
    return box;
}

function main(): void {
    // Error: Cannot infer T - conflicting types Int and String
    Box<Int> result = process(new Int(1), new String("text"));
}

main();
