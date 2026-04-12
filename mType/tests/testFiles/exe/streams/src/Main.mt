// Test: Streams in standalone exe
import * from "../../../../lib/collections/ArrayList.mt";
import * from "../../../../lib/stream/Stream.mt";
import * from "../../../../lib/stream/Streams.mt";
import * from "../../../../lib/functional/Predicate.mt";
import * from "../../../../lib/functional/Function.mt";
import * from "../../../../lib/functional/Consumer.mt";
import * from "../../../../lib/primitives/Int.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        ArrayList<Int> list = new ArrayList<Int>();
        list.add(1);
        list.add(2);
        list.add(3);
        list.add(4);
        list.add(5);
        list.add(6);

        // Filter even numbers
        print("Even numbers:");
        list.stream()
            .filter(value -> value % 2 == 0)
            .forEach(value -> print("  " + value));

        // Map: double each value
        print("Doubled:");
        Int[] doubled = list.stream()
            .map(value -> value * 2)
            .toArray();
        for (Int d : doubled) {
            print("  " + d);
        }

        // Count
        int evenCount = list.stream()
            .filter(value -> value % 2 == 0)
            .count();
        print("Even count: " + evenCount);

        // Chained: filter + map + toArray
        print("Odd * 10:");
        Int[] result = list.stream()
            .filter(value -> value % 2 != 0)
            .map(value -> value * 10)
            .toArray();
        for (Int r : result) {
            print("  " + r);
        }

        print("Streams test passed");
    }
}
