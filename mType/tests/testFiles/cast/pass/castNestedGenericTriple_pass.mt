import * from "../../lib/primitives/Int.mt";

// Test triple nested generic casting Box<Box<Box<T>>>
class Item {
    int value;

    public function Item(int val) {
        value = val;
    }

    public function getValue(): int {
        return value;
    }
}

class Wrapper<T> {
    T data;

    public function setData(T item): void {
        data = item;
    }

    public function getData(): T {
        return data;
    }

    public function display(): void {
        print("Wrapper contains data");
    }
}

function main(): void {
    // Create triple nested: Wrapper<Wrapper<Wrapper<Item>>>
    Item item = new Item(777);

    Wrapper<Item> level1 = new Wrapper<Item>();
    level1.setData(item);

    Wrapper<Wrapper<Item>> level2 = new Wrapper<Wrapper<Item>>();
    level2.setData(level1);

    Wrapper<Wrapper<Wrapper<Item>>> level3 = new Wrapper<Wrapper<Wrapper<Item>>>();
    level3.setData(level2);

    // Unwrap through all levels
    Wrapper<Wrapper<Item>> unwrap1 = level3.getData();
    unwrap1.display();

    Wrapper<Item> unwrap2 = unwrap1.getData();
    unwrap2.display();

    Item unwrap3 = unwrap2.getData();
    print("Triple nested value: " + unwrap3.getValue());

    print("Triple nested generic cast successful");
}

main();
