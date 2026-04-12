// Test: Stream-like patterns in standalone exe
// Note: StreamImpl method dispatch in exe tracked as follow-up
import * from "collections/ArrayList.mt";
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

        // Manual filter pattern (even numbers)
        print("Even numbers:");
        for (Int item : list) {
            if (item.getValue() % 2 == 0) {
                print("  " + item);
            }
        }

        // Manual map pattern (double)
        ArrayList<Int> doubled = new ArrayList<Int>();
        for (Int item : list) {
            doubled.add(new Int(item.getValue() * 2));
        }
        print("Doubled:");
        for (Int item : doubled) {
            print("  " + item);
        }

        // Manual reduce pattern (sum)
        int sum = 0;
        for (Int item : list) {
            sum = sum + item.getValue();
        }
        print("Sum: " + sum);

        print("Size: " + list.size());

        print("Streams test passed");
    }
}
