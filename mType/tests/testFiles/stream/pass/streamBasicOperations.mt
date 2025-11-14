// Test basic stream operations: filter, map, toArray
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/functional/Function.mt";
import * from "../../lib/primitives/Int.mt";



function main(): void {
    // Create a list
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(1);
    list.add(2);
    list.add(3);
    list.add(4);
    list.add(5);

    // Test filter and map
    print("Testing stream filter and map:");
    Stream<Int> stream = list.stream()
        .filter(value->value % 2 == 0)
        .map(value->value * 2);

    Int[] result = stream.toArray();

    print("Filtered even numbers and doubled:");
    for (Int num : result) {
        print(num); // Should print 4, 8
    }

    print("Stream basic operations test passed!");
}
main();
