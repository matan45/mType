// Test iterator on empty collection
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/Iterator.mt";

@Script
function main(): void {
    // Create an empty list
    ArrayList<int> list = new ArrayList<int>();

    // Get iterator
    Iterator<int> iter = list.iterator();

    // Test that hasNext returns false
    print("Testing empty iterator:");
    if (!iter.hasNext()) {
        print("hasNext() correctly returns false for empty collection");
    }

    // Close iterator
    iter.close();

    print("Empty iterator test passed!");
}
