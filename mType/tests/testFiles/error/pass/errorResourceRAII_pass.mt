// Test: RAII pattern with exceptions
// Expected: Resources should be cleaned up when leaving scope
import * from "../../lib/exceptions/Exception.mt";

class FileHandle {
    public String filename;
    public Bool isOpen;

    public constructor(String name) {
        filename = name;
        isOpen = true;
        print("Opened file: " + filename);
    }

    public function close(): void {
        if (isOpen) {
            print("Closing file: " + filename);
            isOpen = false;
        }
    }
}

class ScopedResource {
    public FileHandle handle;

    public constructor(String filename) {
        handle = new FileHandle(filename);
    }

    public function read(): String {
        if (!handle.isOpen) {
            throw new Exception("Cannot read from closed file");
        }
        return "Data from " + handle.filename;
    }

    public function dispose(): void {
        handle.close();
    }
}

function testRAIISuccess(): void {
    print("Test 1: Normal execution with cleanup");
    ScopedResource resource = null;

    try {
        resource = new ScopedResource("data.txt");
        String data = resource.read();
        print("Read: " + data);
    } finally {
        if (resource != null) {
            resource.dispose();
        }
    }
    print("Resource cleaned up successfully");
}

function testRAIIWithException(): void {
    print("Test 2: Exception during operation with cleanup");
    ScopedResource resource = null;

    try {
        resource = new ScopedResource("config.json");
        String data = resource.read();
        print("Read: " + data);
        throw new Exception("Processing error");
    } catch (Exception e) {
        print("Caught exception: " + e.getMessage());
    } finally {
        if (resource != null) {
            resource.dispose();
        }
    }
    print("Resource cleaned up after exception");
}

function testRAIINestedScopes(): void {
    print("Test 3: Nested scopes with multiple resources");
    ScopedResource outer = null;

    try {
        outer = new ScopedResource("outer.txt");
        ScopedResource inner = null;

        try {
            inner = new ScopedResource("inner.txt");
            print("Reading from both resources");
            throw new Exception("Inner operation failed");
        } catch (Exception e) {
            print("Inner exception: " + e.getMessage());
        } finally {
            if (inner != null) {
                inner.dispose();
            }
        }

        print("Outer scope continues");
    } finally {
        if (outer != null) {
            outer.dispose();
        }
    }
    print("All nested resources cleaned up");
}

function main(): void {
    print("Testing RAII pattern with exceptions");
    testRAIISuccess();
    print("---");
    testRAIIWithException();
    print("---");
    testRAIINestedScopes();
    print("RAII test completed");
}

main();
