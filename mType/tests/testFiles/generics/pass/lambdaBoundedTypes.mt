import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda with bounded generic types
interface Comparable {
    function compare(Comparable other): Int;
}

interface Formatter<T> {
    function format(T input): String;
}

class Number implements Comparable {
    Int value;

    public constructor(Int v) {
        value = v;
    }

    public function compare(Comparable other): Int {
        return new Int(0);
    }

    public function getValue(): Int {
        return value;
    }
}

class Processor<T extends Comparable> {
    Formatter<T> formatter;

    public function setFormatter(Formatter<T> f): void {
        formatter = f;
    }

    public function format(T item): String {
        return formatter.format(item);
    }
}

function main(): void {
    Processor<Number> proc = new Processor<Number>();
    proc.setFormatter(n -> new String("Number: " + n.getValue()));

    String result = proc.format(new Number(new Int(42)));
    print(result.toString());
}

main();
