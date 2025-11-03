// Test promise in static field

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Promise in Static Field Test ===");

class Cache {
    static int cachedValue;

    public static function async fetchValue(int value): Promise<Int> {
        print("Fetching value: " + value);
        cachedValue = value;
        return new Int(value);
    }

    public static function getCached(): int {
        return cachedValue;
    }
}

function async testStaticCache(): Promise<Int> {
    print("First fetch");
    Int r1 = await Cache::fetchValue(42);
    print("Cached after first fetch: " + Cache::getCached());

    print("Second fetch");
    Int r2 = await Cache::fetchValue(100);
    print("Cached after second fetch: " + Cache::getCached());

    return r2;
}

function async main(): Promise<Int> {
    Int result = await testStaticCache();
    print("Final result: " + result.getValue());
    return result;
}

main();
