// Test interface segregation principle - clients should not be forced to depend on interfaces they don't use
interface Readable {
    function read(): string;
}

interface Writable {
    function write(string data): void;
}

interface Closable {
    function close(): void;
}

// File implements all three interfaces
class File implements Readable, Writable, Closable {
    string content;
    bool isOpen;

    constructor() {
        this.content = "";
        this.isOpen = true;
    }

    public function read(): string {
        return this.content;
    }

    public function write(string data): void {
        this.content = this.content + data;
    }

    public function close(): void {
        this.isOpen = false;
        print("File closed");
    }
}

// ReadOnlyFile only implements Readable and Closable
class ReadOnlyFile implements Readable, Closable {
    string content;

    constructor(string c) {
        this.content = c;
    }

    public function read(): string {
        return this.content;
    }

    public function close(): void {
        print("ReadOnly file closed");
    }
}

File file = new File();
file.write("Hello, ");
file.write("World!");
print(file.read());
file.close();

ReadOnlyFile roFile = new ReadOnlyFile("Read only content");
print(roFile.read());
roFile.close();

print("Interface segregation successful");
