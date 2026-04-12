// Regression test: MYT-63 — inherited method resolution in standalone exe
// Bug: Library classes deserialized from bytecode had broken parent links,
//      so inherited methods (equals, hashCode, etc.) were not found at runtime.
import * from "collections/HashSet.mt";
import * from "collections/HashMap.mt";
import * from "primitives/Int.mt";
import * from "primitives/String.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Test 1: HashSet.add uses equals() on String (inherited method)
        HashSet<String> set = new HashSet<String>();
        set.add(new String("a"));
        set.add(new String("b"));
        set.add(new String("a"));
        print("PASS: HashSet size = " + set.size());
        print("PASS: contains a = " + set.contains(new String("a")));

        // Test 2: HashMap.get uses equals() on String keys
        HashMap<String, Int> map = new HashMap<String, Int>();
        map.put(new String("key1"), new Int(100));
        map.put(new String("key2"), new Int(200));
        print("PASS: HashMap size = " + map.size());
        print("PASS: get key1 = " + map.get(new String("key1")));

        print("Regression method test passed");
    }
}
