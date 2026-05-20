// Edge: for-each over an array whose element slots may be null. Body uses a
// null guard before dereferencing. Verifies both the null and non-null branches
// execute in their declared iteration order.
class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function main(): void {
    Box?[] arr = new Box?[4];
    arr[0] = new Box(1);
    arr[1] = null;
    arr[2] = new Box(3);
    arr[3] = null;

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
