import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Multiple interface constraints
interface Readable {
    function read(): String;
}

interface Writable {
    function write(String data): void;
}

interface Closeable {
    function close(): void;
}

class Stream implements Readable, Writable, Closeable {
    String buffer;

    public constructor() {
        buffer = new String("");
    }

    public function read(): String {
        return buffer;
    }

    public function write(String data): void {
        buffer = data;
    }

    public function close(): void {
        print("Stream closed");
    }
}

class Manager<T extends Readable> {
    T resource;

    public function setResource(T r): void {
        resource = r;
    }

    public function readData(): String {
        return resource.read();
    }
}

function main(): void {
    Stream stream = new Stream();
    stream.write(new String("Data"));

    Manager<Stream> manager = new Manager<Stream>();
    manager.setResource(stream);
    print("Read: " + manager.readData());

    stream.close();
}

main();
