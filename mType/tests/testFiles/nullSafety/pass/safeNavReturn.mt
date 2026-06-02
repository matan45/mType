// MYT-374: a safe-navigation result can be returned directly — the value
// (field or null) flows into RETURN_VALUE, and the declared return type must be
// nullable. Covers both obj?.field and arr[i]?.field in return position.

class Inner {
    public string text;
    constructor(string t) { this.text = t; }
}

class Box {
    public Inner inner;
    constructor(Inner i) { this.inner = i; }
}

function getInner(Box? b): Inner? {
    return b?.inner;
}

function firstInner(Box[] boxes): Inner? {
    return boxes[0]?.inner;
}

function main(): void {
    Box present = new Box(new Inner("hello"));
    Box? absent = null;

    Inner? r1 = getInner(present);
    if (r1 != null) {
        print("r1: " + r1.text);
    } else {
        print("r1 null");
    }

    Inner? r2 = getInner(absent);
    if (r2 == null) {
        print("r2 null");
    }

    Box[] boxes = new Box[1];
    boxes[0] = new Box(new Inner("world"));
    Inner? r3 = firstInner(boxes);
    if (r3 != null) {
        print("r3: " + r3.text);
    }
}

main();

// Expected output:
// r1: hello
// r2 null
// r3: world
