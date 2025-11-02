// Test finally for resource cleanup
import * from "../../lib/exceptions/Exception.mt";

class Resource {
    private String name;
    private bool isOpen;

    constructor(String name) {
        this.name = name;
        this.isOpen = false;
    }

    open(): void {
        print("Opening resource: " + this.name);
        this.isOpen = true;
    }

    close(): void {
        if (this.isOpen) {
            print("Closing resource: " + this.name);
            this.isOpen = false;
        }
    }

    process(): void {
        if (!this.isOpen) {
            throw new Exception("Resource not open: " + this.name);
        }
        print("Processing resource: " + this.name);
    }
}

function testResourceCleanupSuccess(): void {
    Resource r = new Resource("File1");

    try {
        r.open();
        r.process();
    } finally {
        r.close();
    }
}

function testResourceCleanupException(): void {
    Resource r = new Resource("File2");

    try {
        r.open();
        r.process();
        throw new Exception("Error during processing");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    } finally {
        r.close();
    }
}

function testMultipleResourceCleanup(): void {
    Resource r1 = new Resource("File3");
    Resource r2 = new Resource("File4");

    try {
        r1.open();
        try {
            r2.open();
            r1.process();
            r2.process();
        } finally {
            r2.close();
        }
    } finally {
        r1.close();
    }
}

function main(): void {
    print("Testing resource cleanup with finally");

    testResourceCleanupSuccess();
    testResourceCleanupException();
    testMultipleResourceCleanup();

    print("Test completed");
}

main();
