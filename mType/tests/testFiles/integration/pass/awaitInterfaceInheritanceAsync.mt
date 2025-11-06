// Test: Interface inheritance with async methods
// @Script

interface AsyncReadable {
    function async read() : Promise<String>;
}

interface AsyncWritable {
    function async write(String data) : Promise<void>;
}

interface AsyncStream implements AsyncReadable, AsyncWritable {
    function async flush() : Promise<void>;
}

class MemoryStream implements AsyncStream {
    private String buffer;

    constructor() {
        this.buffer = "";
    }

    public function async read() : Promise<String> {
        await delay(10);
        String result = this.buffer;
        print("Read: " + result);
        return result;
    }

    public function async write(String data) : Promise<void> {
        await delay(10);
        this.buffer = this.buffer + data;
        print("Written: " + data);
    }

    public function async flush() : Promise<void> {
        await delay(10);
        print("Flushing buffer");
        this.buffer = "";
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async testReadable(AsyncReadable stream) : Promise<String> {
    return await stream.read();
}

function async testWritable(AsyncWritable stream) : Promise<void> {
    await stream.write("Test");
}

function async main() : Promise<void> {
    AsyncStream stream = new MemoryStream();

    await stream.write("Hello");
    await stream.write(" World");

    String content = await stream.read();
    assert(content == "Hello World", "Should concatenate writes");

    await stream.flush();
    String afterFlush = await stream.read();
    assert(afterFlush == "", "Should be empty after flush");

    // Test polymorphism
    await testWritable(stream);
    String result = await testReadable(stream);
    assert(result == "Test", "Polymorphic async should work");
}
