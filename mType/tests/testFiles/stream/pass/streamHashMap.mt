// Test: HashMap stream operations
// Tests stream(), streamEntries(), and streamValues() on HashMap

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/stream/Stream.mt";

function main(): void {
    HashMap<String, Int> map = new HashMap<String, Int>();
    map.put(new String("one"), new Int(1));
    map.put(new String("two"), new Int(2));
    map.put(new String("three"), new Int(3));
    map.put(new String("four"), new Int(4));
    map.put(new String("five"), new Int(5));

    print("=== Stream over keys ===");
    Stream<String> keyStream = map.stream();
    String[] keys = keyStream.toArray();
    print("Total keys: " + keys.length);

    print("=== Stream over values ===");
    Stream<Int> valueStream = map.streamValues();
    Int[] values = valueStream.toArray();
    int sum = 0;
    for (Int val : values) {
        sum = sum + val.getValue();
    }
    print("Sum of values: " + sum);

    print("=== Stream over entries ===");
    Stream<MapEntry<String, Int>> entryStream = map.streamEntries();
    int count = 0;
    entryStream.forEach(entry -> {
        count = count + 1;
        return;
    });
    print("Total entries: " + count);

    print("HashMap stream test passed!");
}

main();
