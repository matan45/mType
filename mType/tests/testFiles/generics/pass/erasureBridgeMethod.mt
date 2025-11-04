import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Bridge method behavior with generic overrides
class Processor<T> {
    public function process(T item): String {
        return new String("Base process");
    }
}

class StringProcessor extends Processor<String> {
    public function process(String item): String {
        return new String("String: " + item);
    }
}

class IntProcessor extends Processor<Int> {
    public function process(Int item): String {
        return new String("Int: " + item.value);
    }
}

function main(): void {
    StringProcessor sp = new StringProcessor();
    String result1 = sp.process(new String("test"));
    print(result1.toString());

    IntProcessor ip = new IntProcessor();
    String result2 = ip.process(new Int(42));
    print(result2.toString());
}

main();
