import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Intersection type constraints (T extends A & B)
interface Printable {
    function toString(): String;
}

interface Comparable {
    function compareTo(Comparable other): Int;
}

class Element extends Printable, Comparable {
    Int value;

    public function Element(Int v) {
        value = v;
    }

    public function toString(): String {
        return new String("Element: " + value);
    }

    public function compareTo(Comparable other): Int {
        return new Int(0);
    }
}

class Container<T extends Printable> {
    T item;

    public function setItem(T i): void {
        item = i;
    }

    public function print(): void {
        print(item.toString());
    }
}

function main(): void {
    Container<Element> container = new Container<Element>();
    container.setItem(new Element(new Int(42)));
    container.print();
}

main();
