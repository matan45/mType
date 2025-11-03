// Test: Interface inheritance with async methods
// @Script

interface AsyncReadable {
    async read() : Promise<String>;
}

interface AsyncWritable {
    async write(data: String) : Promise<Void>;
}

interface AsyncStream : AsyncReadable, AsyncWritable {
    async flush() : Promise<Void>;
}

class MemoryStream implements AsyncStream {
    private buffer: String;

    constructor() {
        this.buffer = "";
    }

    async read() : Promise<String> {
        await delay(10);
        let result = this.buffer;
        print("Read: " + result);
        return result;
    }

    async write(data: String) : Promise<Void> {
        await delay(10);
        this.buffer = this.buffer + data;
        print("Written: " + data);
    }

    async flush() : Promise<Void> {
        await delay(10);
        print("Flushing buffer");
        this.buffer = "";
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async testReadable(stream: AsyncReadable) : Promise<String> {
    return await stream.read();
}

async testWritable(stream: AsyncWritable) : Promise<Void> {
    await stream.write("Test");
}

async main() : Promise<Void> {
    let stream: AsyncStream = new MemoryStream();

    await stream.write("Hello");
    await stream.write(" World");

    let content = await stream.read();
    assert(content == "Hello World", "Should concatenate writes");

    await stream.flush();
    let afterFlush = await stream.read();
    assert(afterFlush == "", "Should be empty after flush");

    // Test polymorphism
    await testWritable(stream);
    let result = await testReadable(stream);
    assert(result == "Test", "Polymorphic async should work");
}
