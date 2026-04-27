import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

// Test ArrayList<ArrayList<Int>>: populate, access, iterate, flatten, sum.
function main(): void {
    print("Testing ArrayList of ArrayList");

    ArrayList<ArrayList<Int>> outer = new ArrayList<ArrayList<Int>>();

    ArrayList<Int> row0 = new ArrayList<Int>();
    row0.add(new Int(1));
    row0.add(new Int(2));
    row0.add(new Int(3));

    ArrayList<Int> row1 = new ArrayList<Int>();
    row1.add(new Int(10));
    row1.add(new Int(20));

    ArrayList<Int> row2 = new ArrayList<Int>();
    row2.add(new Int(100));

    outer.add(row0);
    outer.add(row1);
    outer.add(row2);

    print("Outer size: " + outer.size());
    print("Row 0 size: " + outer.get(0).size());
    print("Row 1 size: " + outer.get(1).size());
    print("Row 2 size: " + outer.get(2).size());

    // Direct access
    print("outer[0][0] = " + outer.get(0).get(0).getValue());
    print("outer[1][1] = " + outer.get(1).get(1).getValue());
    print("outer[2][0] = " + outer.get(2).get(0).getValue());

    // Iterate and flatten + sum
    int sum = 0;
    for (int i = 0; i < outer.size(); i++) {
        ArrayList<Int> row = outer.get(i);
        for (int j = 0; j < row.size(); j++) {
            sum = sum + row.get(j).getValue();
        }
    }
    print("Flattened sum: " + sum);

    print("Recursive list of list test completed");
}

main();
