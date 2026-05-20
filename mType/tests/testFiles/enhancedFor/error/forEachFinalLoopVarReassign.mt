// Error: explicit `final` on the for-each loop variable must forbid
// reassignment in the body. Mirrors errorFinalLocalIncrement_error.mt for
// for-each scope. Drop this fixture if `final` isn't accepted in for-each
// headers — observed during first build.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(new Int(1));
    list.add(new Int(2));

    for (final Int v : list) {
        v = new Int(99);
        print(v.getValue());
    }
}

main();
