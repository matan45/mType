// Edge: Box[] elements default to null; for-each binds each slot to a
// Box? loop var. Verifies both null and non-null slots iterate in order.
// Mirrors the implicit-null pattern from
// nullSafety/pass/arrayElementNullThenReassign.mt — `Box?[]` declaration
// syntax isn't accepted (parser rejects with "Expected variable name").
class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function main(): void {
    Box[] arr = new Box[4];
    arr[0] = new Box(1);
    // arr[1] left implicitly null
    arr[2] = new Box(3);
    // arr[3] left implicitly null

    int nulls = 0;
    int sum = 0;
    for (Box? b : arr) {
        if (b == null) {
            nulls = nulls + 1;
        } else {
            sum = sum + b.value;
        }
    }
    print("sum=" + sum + " nulls=" + nulls);
}

main();

// Expected output:
// sum=4 nulls=2
