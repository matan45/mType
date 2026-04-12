// Test: Streams in standalone exe
import * from "collections/ArrayList.mt";
import * from "stream/Stream.mt";
import * from "stream/Streams.mt";
import * from "functional/Predicate.mt";
import * from "functional/Function.mt";
import * from "functional/Consumer.mt";
import * from "primitives/Int.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        ArrayList<Int> list = new ArrayList<Int>();
        list.add(new Int(1));
        list.add(new Int(2));
        list.add(new Int(3));
        list.add(new Int(4));
        list.add(new Int(5));

        // forEach
        print("All items:");
        list.stream().forEach(value -> print("  " + value));

        // filter + count
        int evenCount = list.stream()
            .filter(value -> value % 2 == 0)
            .count();
        print("Even count: " + evenCount);

        // filter + map + toArray
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
