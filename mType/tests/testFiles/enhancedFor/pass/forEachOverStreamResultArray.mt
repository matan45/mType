// Test for-each iterating over the array result of a stream pipeline
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing for-each over stream result:");

    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    list.add(new Int(5));
    list.add(new Int(6));

    Int[] arr = list.stream().filter(v -> v.getValue() % 2 == 0).toArray();
    int sum = 0;
    for (Int x : arr) {
        sum = sum + x.getValue();
        print(x);
    }
    print("sum: " + sum);

    print("for-each over stream result test passed!");
}
main();
