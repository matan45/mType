// MYT-374: arr[i]?.field = value writes through to a non-null element and is a
// null no-op (no crash) when the element is null. The safe target is routed off
// the SoA ARRAY_SET_FIELD fast path so the element is materialised and
// null-checked first.

class Inner {
    public string text;
    constructor(string t) { this.text = t; }
}

class Box {
    public Inner inner;
    constructor(Inner i) { this.inner = i; }
}

function main(): void {
    Box[] boxes = new Box[2];
    boxes[0] = new Box(new Inner("old"));
    // boxes[1] stays null

    boxes[0]?.inner = new Inner("updated"); // element non-null -> writes
    boxes[1]?.inner = new Inner("ignored"); // element null -> skipped, no crash

    print("boxes[0]: " + boxes[0].inner.text);
    print("done");
}

main();

// Expected output:
// boxes[0]: updated
// done
