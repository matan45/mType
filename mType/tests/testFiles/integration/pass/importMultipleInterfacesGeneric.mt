// Test: Import multiple generic interfaces
// @Script

import "modules/ReadableInterface.mt";
import "modules/WritableInterface.mt";

class Buffer<T> implements Readable<T>, Writable<T> {
    private T[] data;
    private Int position;

    constructor() {
        this.data = [];
        this.position = 0;
    }

    public function write(item: T) : void {
        this.data.push(item);
        print("Written item at position " + this.data.length().toString());
    }

    public function read() : T {
        if (this.position < this.data.length()) {
            T item = this.data[this.position];
            this.position = this.position + 1;
            return item;
        }
        return this.data[0]; // Default
    }

    public function reset() : void {
        this.position = 0;
    }

    public function hasMore() : Bool {
        return this.position < this.data.length();
    }
}

function main() : void {
    Buffer<String> buffer = new Buffer<String>();

    // Test as Writable
    Writable<String> writable = buffer;
    writable.write("Hello");
    writable.write("World");
    writable.write("Test");

    // Test as Readable
    Readable<String> readable = buffer;
    buffer.reset();

    String item1 = readable.read();
    print("Read: " + item1);
    assert(item1 == "Hello", "Should read first item");

    String item2 = readable.read();
    print("Read: " + item2);
    assert(item2 == "World", "Should read second item");

    print("Multiple interface test passed");
}
