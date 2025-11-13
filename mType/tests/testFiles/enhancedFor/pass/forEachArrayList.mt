// Test enhanced for-loop with ArrayList
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";


function main(): void {
    // Create a list
    ArrayList<String> list = new ArrayList<String>();
    list.add("Hello");
    list.add("World");
    list.add("from");
    list.add("mType");

    // Test enhanced for-loop
    print("Testing enhanced for-loop with ArrayList:");
    for (String word : list) {
        print(word);
    }

    print("Enhanced for-loop test passed!");
}
main();
