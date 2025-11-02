// Exception with resource cleanup pattern test
interface Function {
    function apply(int x) : int;
}

class Resource {
    String name;
    bool isOpen;

    function init(String n) {
        this.name = n;
        this.isOpen = true;
        print("Resource '" + this.name + "' opened");
    }

    function close() {
        if (this.isOpen) {
            this.isOpen = false;
            print("Resource '" + this.name + "' closed");
        }
    }

    function use(int value) : int {
        if (!this.isOpen) {
            throw "Resource is closed";
        }
        return value * 2;
    }
}

print("=== Resource Cleanup Test ===");

Function safeProcessor = x -> {
    Resource r = new Resource("Lambda-Resource");
    try {
        if (x < 0) {
            throw "Invalid input";
        }
        int result = r.use(x);
        return result;
    } catch (String e) {
        print("Error: " + e);
        return -1;
    } finally {
        r.close();
    }
};

print("Process(10): " + safeProcessor.apply(10));
print("Process(-5): " + safeProcessor.apply(-5));
print("Process(20): " + safeProcessor.apply(20));

// Multiple resources
Function multiResource = x -> {
    Resource r1 = new Resource("R1");
    Resource r2 = new Resource("R2");
    try {
        int result = r1.use(x);
        result = r2.use(result);
        return result;
    } finally {
        r2.close();
        r1.close();
    }
};

print("Multi(5): " + multiResource.apply(5));

print("Resource cleanup complete");
