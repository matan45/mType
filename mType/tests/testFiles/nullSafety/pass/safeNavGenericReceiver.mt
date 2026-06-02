// Test: ?. works on a generic nullable receiver (Box<T>?) for both field and
// method access, short-circuiting to null when the receiver is null.

class Address {
    string city;
    constructor(string c) { city = c; }
    public function getCity(): string { return city; }
}

class Box<T> {
    public T value;
    constructor(T v) { value = v; }
    public function get(): T { return value; }
}

function main(): void {
    Box<Address>? box = new Box<Address>(new Address("Tokyo"));

    Address? viaMethod = box?.get();
    if (viaMethod != null) {
        print("Method: " + viaMethod.getCity());
    } else {
        print("null");
    }

    Address? viaField = box?.value;
    if (viaField != null) {
        print("Field: " + viaField.getCity());
    } else {
        print("null");
    }

    Box<Address>? empty = null;
    Address? gone = empty?.get();
    if (gone == null) {
        print("short-circuited");
    }
}

main();

// Expected output:
// Method: Tokyo
// Field: Tokyo
// short-circuited
