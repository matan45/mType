// Error: source has element type `Box?` but the loop variable is declared
// non-nullable `Box`. The type-checker must reject — binding a possibly-null
// slot to a non-nullable variable is unsound.
class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function main(): void {
    Box?[] arr = new Box?[2];
    arr[0] = new Box(1);
    arr[1] = null;

    for (Box b : arr) {
        print(b.value);
    }
}

main();
