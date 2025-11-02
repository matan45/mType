import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda with bounded generic types
interface Comparable {
    function compare(Comparable other): Int;
}

class Number extends Comparable {
    Int value;

    public function Number(Int v) {
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
    function(T): String formatter;

    public function setFormatter(function(T): String f): void {
        formatter = f;
    }

    public function format(T item): String {
        return formatter(item);
    }
}

function main(): void {
    Processor<Number> proc = new Processor<Number>();
    proc.setFormatter(function(Number n): String {
        return new String("Number: " + n.getValue());
    });

    String result = proc.format(new Number(new Int(42)));
    print(result);
}

main();
