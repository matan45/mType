// Test: Resource cleanup patterns
// Expected: Pass - demonstrates resource management patterns
import * from "../../lib/exceptions/Exception.mt";
class Resource {
    private string name;
    private bool isOpen;

    public constructor(string name) {
        this.name = name;
        this.isOpen = false;
    }

    public function open(): void {
        if (this.isOpen) {
            print("Resource " + this.name + " already open");
            return;
        }
        print("Opening resource: " + this.name);
        this.isOpen = true;
    }

    public function close(): void {
        if (!this.isOpen) {
            print("Resource " + this.name + " already closed");
            return;
        }
        print("Closing resource: " + this.name);
        this.isOpen = false;
    }

    public function isResourceOpen(): bool {
        return this.isOpen;
    }

    public function use(): void {
        if (!this.isOpen) {
            print("Error: Cannot use closed resource " + this.name);
            return;
        }
        print("Using resource: " + this.name);
    }
}

class ResourceManager {
    public function useResource(Resource r): void {
        r.open();
        try {
            r.use();
            r.use();
        } finally {
            r.close();
        }
    }

    public function useResourceWithError(Resource r, bool throwError): void {
        r.open();
        try {
            r.use();
            if (throwError) {
                print("Throwing error during resource use");
                throw new Exception("ResourceError");
            }
            r.use();
        } catch (Exception e) {
            print("Caught error: " + e.getMessage());
        } finally {
            print("Finally block - cleanup");
            r.close();
        }
    }
}

// Test resource cleanup
ResourceManager manager = new ResourceManager();

print("Test 1: Normal resource usage");
Resource r1 = new Resource("File1");
manager.useResource(r1);
print("Resource open: " + r1.isResourceOpen());

print("\nTest 2: Resource usage with error");
Resource r2 = new Resource("File2");
manager.useResourceWithError(r2, true);
print("Resource open: " + r2.isResourceOpen());

print("\nTest 3: Resource usage without error");
Resource r3 = new Resource("File3");
manager.useResourceWithError(r3, false);
print("Resource open: " + r3.isResourceOpen());

print("\nTest 4: Manual cleanup");
Resource r4 = new Resource("File4");
r4.open();
r4.use();
r4.close();
r4.use();  // Try to use closed resource
