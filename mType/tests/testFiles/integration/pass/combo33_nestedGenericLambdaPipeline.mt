// Combo 33: Nested generic ArrayList<ArrayList<Int>> + lambda pipeline
// Tests: flatten, transform, and sum through Function/Predicate lambdas

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/functional/Function.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";

function flatten(ArrayList<ArrayList<Int>> matrix): ArrayList<Int> {
    ArrayList<Int> flat = new ArrayList<Int>();
    for (int i = 0; i < matrix.size(); i++) {
        ArrayList<Int> row = matrix.get(i);
        for (int j = 0; j < row.size(); j++) {
            flat.add(row.get(j));
        }
    }
    return flat;
}

function transform(ArrayList<Int> source, Function<Int, Int> fn): ArrayList<Int> {
    ArrayList<Int> dest = new ArrayList<Int>();
    for (int i = 0; i < source.size(); i++) {
        dest.add(fn.apply(source.get(i)));
    }
    return dest;
}

function sumIf(ArrayList<Int> source, Predicate<Int> keep): int {
    int total = 0;
    for (int i = 0; i < source.size(); i++) {
        Int v = source.get(i);
        if (keep.test(v)) {
            total = total + v.getValue();
        }
    }
    return total;
}

function main(): void {
    print("=== Combo 33: Nested Generic Lambda Pipeline ===");

    ArrayList<ArrayList<Int>> matrix = new ArrayList<ArrayList<Int>>();

    ArrayList<Int> row1 = new ArrayList<Int>();
    row1.add(new Int(1)); row1.add(new Int(2)); row1.add(new Int(3));
    matrix.add(row1);

    ArrayList<Int> row2 = new ArrayList<Int>();
    row2.add(new Int(4)); row2.add(new Int(5));
    matrix.add(row2);

    ArrayList<Int> row3 = new ArrayList<Int>();
    row3.add(new Int(6)); row3.add(new Int(7)); row3.add(new Int(8)); row3.add(new Int(9));
    matrix.add(row3);

    print("rows=" + matrix.size());

    ArrayList<Int> flat = flatten(matrix);
    print("flat count=" + flat.size());

    Function<Int, Int> doubler = x -> new Int(x.getValue() * 2);
    ArrayList<Int> doubled = transform(flat, doubler);
    print("doubled count=" + doubled.size());

    int rawSum = sumIf(flat, n -> true);
    print("rawSum=" + rawSum);

    int doubledSum = sumIf(doubled, n -> true);
    print("doubledSum=" + doubledSum);

    int evensOnly = sumIf(flat, n -> n.getValue() % 2 == 0);
    print("evens=" + evensOnly);

    int doubledOver10 = sumIf(doubled, n -> n.getValue() > 10);
    print("doubledOver10=" + doubledOver10);

    print("=== Combo 33 Complete ===");
}

main();
