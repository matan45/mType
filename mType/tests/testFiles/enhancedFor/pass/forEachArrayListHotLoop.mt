// JIT variant: drive the for-each over ArrayList<Int> through a hot function
// so the second CI pass actually compiles and OSRs through the iterator
// dispatch. Pairs with forEachArrayList.mt for the interpreter shape.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function sumList(ArrayList<Int> list): int {
    int s = 0;
    for (Int v : list) {
        s = s + v.getValue();
    }
    return s;
}

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));
    list.add(new Int(3));
    list.add(new Int(4));
    list.add(new Int(5));

    int N = 50000;
    int total = 0;
    for (int i = 0; i < N; i = i + 1) {
        total = total + sumList(list);
    }
    print("total=" + total);
}

main();

// Expected output:
// total=750000
