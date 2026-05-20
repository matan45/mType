// CANARY (MYT-NEW): nested generic for-each loses inner element type.
// Outer for-each over ArrayList<ArrayList<Int>> binds `row` as ArrayList<Int>
// (its declared loop-variable type), but the inner for-each then types `v` as
// Object and rejects with MT-E2007. See
// memory:project_foreach_loses_nested_generic_type. Keep failing until the
// fix lands.
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    ArrayList<ArrayList<Int>> matrix = new ArrayList<ArrayList<Int>>();

    ArrayList<Int> row0 = new ArrayList<Int>();
    row0.add(new Int(1));
    row0.add(new Int(2));
    matrix.add(row0);

    ArrayList<Int> row1 = new ArrayList<Int>();
    row1.add(new Int(10));
    row1.add(new Int(20));
    matrix.add(row1);

    int sum = 0;
    for (ArrayList<Int> row : matrix) {
        for (Int v : row) {
            sum = sum + v.getValue();
        }
    }
    print("sum=" + sum);
}

main();

// Expected output (when MYT-NEW is fixed):
// sum=33
