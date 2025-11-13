// Test stream forEach operation
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Consumer.mt";
import * from "../../lib/primitives/String.mt";


function main(): void {
    // Create a list
    ArrayList<String> list = new ArrayList<String>();
    list.add("Alpha");
    list.add("Beta");
    list.add("Gamma");

    // Test forEach
    print("Testing stream forEach:");
    list.stream().forEach(val -> print("Item: " + val));

    print("Stream forEach test passed!");
}

main();
