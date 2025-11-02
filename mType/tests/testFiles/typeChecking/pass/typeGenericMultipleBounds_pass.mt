import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test multiple bounds on type parameters
interface Comparable<T> {
    function compareTo(T other): int;
}

interface Serializable {
    function serialize(): string;
}

class DataItem implements Comparable<DataItem>, Serializable {
    int id;
    string name;

    constructor(int itemId, string itemName) {
        id = itemId;
        name = itemName;
    }

    public function compareTo(DataItem other): int {
        if (id < other.id) {
            return -1;
        } else if (id > other.id) {
            return 1;
        }
        return 0;
    }

    public function serialize(): string {
        return "DataItem{id=" + id + ", name=" + name + "}";
    }

    public function getId(): int {
        return id;
    }

    public function getName(): string {
        return name;
    }
}

// Generic class with multiple bounds
class BoundedContainer<T extends Comparable<T>, T extends Serializable> {
    T item;

    constructor(T i) {
        item = i;
    }

    public function getItem(): T {
        return item;
    }

    public function compare(T other): int {
        return item.compareTo(other);
    }

    public function toString(): string {
        return item.serialize();
    }
}

// Generic function with multiple bounds
function <T extends Comparable<T>, T extends Serializable> processItem(T item): string {
    return "Processing: " + item.serialize();
}

function main(): void {
    print("Testing multiple bounds on type parameters");

    DataItem item1 = new DataItem(1, "First");
    DataItem item2 = new DataItem(2, "Second");
    DataItem item3 = new DataItem(3, "Third");

    BoundedContainer<DataItem> container = new BoundedContainer<DataItem>(item1);
    print("Container item: " + container.toString());

    int cmp1 = container.compare(item2);
    print("Comparing item1 to item2: " + cmp1);

    int cmp2 = item2.compareTo(item3);
    print("Comparing item2 to item3: " + cmp2);

    print(processItem<DataItem>(item1));
    print(processItem<DataItem>(item2));
    print(processItem<DataItem>(item3));

    print("Multiple bounds test completed");
}

main();
