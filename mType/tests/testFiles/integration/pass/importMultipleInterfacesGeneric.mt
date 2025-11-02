// Test: Import multiple generic interfaces
// @Script

import "modules/ReadableInterface.mt";
import "modules/WritableInterface.mt";

class Buffer<T> : Readable<T>, Writable<T> {
    private data: T[];
    private position: Int;

    constructor() {
        this.data = [];
        this.position = 0;
    }

    write(item: T) : Void {
        this.data.push(item);
        print("Written item at position " + this.data.length().toString());
    }

    read() : T {
        if (this.position < this.data.length()) {
            let item = this.data[this.position];
            this.position = this.position + 1;
            return item;
        }
        return this.data[0]; // Default
    }

    reset() : Void {
        this.position = 0;
    }

    hasMore() : Bool {
        return this.position < this.data.length();
    }
}

main() : Void {
    let buffer: Buffer<String> = new Buffer<String>();

    // Test as Writable
    let writable: Writable<String> = buffer;
    writable.write("Hello");
    writable.write("World");
    writable.write("Test");

    // Test as Readable
    let readable: Readable<String> = buffer;
    buffer.reset();

    let item1 = readable.read();
    print("Read: " + item1);
    assert(item1 == "Hello", "Should read first item");

    let item2 = readable.read();
    print("Read: " + item2);
    assert(item2 == "World", "Should read second item");

    print("Multiple interface test passed");
}
