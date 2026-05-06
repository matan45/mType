// Edge: loop variable shadows an outer local with the same name. The outer
// `x` must remain unchanged after the loop exits.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    int x = 999;
    ArrayList<Int> a = new ArrayList<Int>();
    a.add(new Int(1));
    a.add(new Int(2));

    for (Int x : a) {
        print(x.getValue());
    }
    print("outer=" + x);
}

main();
