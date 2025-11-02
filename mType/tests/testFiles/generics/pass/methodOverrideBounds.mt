import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Override with different type bounds
interface Printable {
    function print(): void;
}

class Base {
    public function <T extends Printable> process(T item): void {
        item.print();
    }
}

class Item extends Printable {
    String name;

    public function Item(String n) {
        name = n;
    }

    public function print(): void {
        print("Item: " + name);
    }
}

class Derived extends Base {
    public function <T extends Printable> process(T item): void {
        print("Derived processing");
        item.print();
    }
}

function main(): void {
    Derived d = new Derived();
    d.process(new Item(new String("Test")));
}

main();
