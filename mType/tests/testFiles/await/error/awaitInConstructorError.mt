// Error: Cannot use await in constructor

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Constructor Error Test ===");

function async loadData(): Promise<Int> {
    print("Loading data");
    return new Int(42);
}

class DataContainer {
    int value;

    // ERROR: Constructors cannot be async, cannot use await
    public constructor() {
        Int data = await loadData();
        this.value = data.getValue();
    }

    public function getValue(): int {
        return this.value;
    }
}

DataContainer container = new DataContainer();
print("Value: " + container.getValue());
