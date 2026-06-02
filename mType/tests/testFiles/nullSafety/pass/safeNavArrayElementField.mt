// MYT-374: arr[i]?.field short-circuits to null when the element is null,
// instead of dereferencing it via the SoA ARRAY_GET_FIELD fast path.

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
    boxes[0] = new Box(new Inner("hello"));
    // boxes[1] stays null

    Inner? a = boxes[0]?.inner;
    if (a != null) {
        print("a: " + a.text);
    } else {
        print("a null");
    }

    Inner? b = boxes[1]?.inner;
    if (b == null) {
        print("b short-circuited");
    } else {
        print("b: " + b.text);
    }
}

main();

// Expected output:
// a: hello
// b short-circuited
