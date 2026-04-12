// Regression test: MYT-63 — inherited method resolution in standalone exe
// Also tests MYT-65 — equals() value comparison via HashSet/HashMap
import * from "collections/ArrayList.mt";
import * from "collections/HashSet.mt";
import * from "collections/HashMap.mt";
import * from "collections/Stack.mt";
import * from "primitives/Int.mt";
import * from "primitives/String.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Test 1: ArrayList basic operations
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

        // Test 3: HashSet with equals (MYT-65)
        HashSet<String> set = new HashSet<String>();
        set.add(new String("a"));
        set.add(new String("b"));
        set.add(new String("a"));
        print("PASS: HashSet size = " + set.size());
        print("PASS: contains a = " + set.contains(new String("a")));

        // Test 4: HashMap with equals (MYT-65)
        HashMap<String, Int> map = new HashMap<String, Int>();
        map.put(new String("key1"), new Int(100));
        map.put(new String("key2"), new Int(200));
        print("PASS: HashMap size = " + map.size());
        print("PASS: get key1 = " + map.get(new String("key1")));

        print("Regression method test passed");
    }
}
