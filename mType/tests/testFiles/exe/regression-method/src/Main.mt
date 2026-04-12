// Regression test: MYT-63 — inherited method resolution in standalone exe
// Tests that methods are found on deserialized classes
import * from "collections/ArrayList.mt";
import * from "collections/Stack.mt";
import * from "primitives/Int.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Test 1: ArrayList basic operations (method resolution)
        ArrayList<Int> list = new ArrayList<Int>();
        list.add(new Int(10));
        list.add(new Int(20));
        list.add(new Int(30));
        print("PASS: ArrayList size = " + list.size());
        print("PASS: get(1) = " + list.get(1));

        // Test 2: Stack operations
        Stack<Int> stack = new Stack<Int>();
        stack.push(new Int(100));
        stack.push(new Int(200));
        print("PASS: Stack peek = " + stack.peek());
        print("PASS: Stack size = " + stack.size());

        // Test 3: ArrayList iteration (iterator method resolution)
        print("PASS: iteration:");
        for (Int item : list) {
            print("  " + item);
        }

        print("Regression method test passed");
    }
}
