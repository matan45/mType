// Test promise as class field

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Promise in Class Field Test ===");

class DataLoader {
    Promise<Int>? pendingData;
    int loadCount;

    public constructor() {
        this.pendingData = null;
        this.loadCount = 0;
    }

    public function async loadData(int value): Promise<Int> {
        print("Loading data: " + value);
        this.loadCount = this.loadCount + 1;
        return new Int(value * 2);
    }

    public function async fetchAndStore(int value): Promise<Int> {
        print("Fetching value: " + value);
        Promise<Int> promise = this.loadData(value);
        this.pendingData = promise;

        Int result = await promise;
        print("Stored and retrieved: " + result);
        return result;
    }

    public function getLoadCount(): int {
        return this.loadCount;
    }
}

function async main(): Promise<Int> {
    DataLoader loader = new DataLoader();

    Int r1 = await loader.fetchAndStore(5);
    print("Result 1: " + r1);

    Int r2 = await loader.fetchAndStore(10);
    print("Result 2: " + r2);

    print("Total loads: " + loader.getLoadCount());

    return r2;
}

main();
