// Test: Resource cleanup after failed cast operations
// Expected: Pass - demonstrates proper cleanup when casts fail

import * from "../../lib/exceptions/Exception.mt";

class CleanupException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class Resource {
    protected string name;
    protected bool isOpen;

    public constructor(string name) {
        this.name = name;
        this.isOpen = true;
        print("Resource opened: " + this.name);
    }

    public void close() {
        if (this.isOpen) {
            print("Resource closed: " + this.name);
            this.isOpen = false;
        }
    }

    public bool getIsOpen() {
        return this.isOpen;
    }

    public string getName() {
        return this.name;
    }
}

class FileResource extends Resource {
    private string path;

    public constructor(string name, string path) : super(name) {
        this.path = path;
    }

    public string getPath() {
        return this.path;
    }

    public void read() {
        print("Reading file: " + this.path);
    }
}

class DatabaseResource extends Resource {
    private string connectionString;

    public constructor(string name, string conn) : super(name) {
        this.connectionString = conn;
    }

    public string getConnectionString() {
        return this.connectionString;
    }

    public void query() {
        print("Querying database: " + this.connectionString);
    }
}

// Test 1: Cleanup in finally after failed cast
function processCastWithCleanup(Resource r): string {
    try {
        if (r instanceof FileResource) {
            FileResource fr = (FileResource)r;
            fr.read();
            return "File processed";
        } else {
            throw new CleanupException("Not a file resource");
        }
    } catch (CleanupException e) {
        print("Exception: " + e.getMessage());
        return "Failed to process";
    } finally {
        r.close();
        print("Cleanup completed");
    }
}

// Test 2: Multiple resource cleanup after mixed casts
function processMultipleResources(Resource r1, Resource r2): string {
    try {
        if (r1 instanceof FileResource) {
            FileResource fr = (FileResource)r1;
            fr.read();
        } else {
            throw new CleanupException("r1 is not a file");
        }

        if (r2 instanceof DatabaseResource) {
            DatabaseResource dr = (DatabaseResource)r2;
            dr.query();
        } else {
            throw new CleanupException("r2 is not a database");
        }

        return "Both resources processed";
    } catch (CleanupException e) {
        print("Error during processing: " + e.getMessage());
        return "Processing failed";
    } finally {
        print("Cleaning up all resources");
        r1.close();
        r2.close();
    }
}

// Test 3: Nested try with cleanup at each level
function nestedCastCleanup(Resource r): string {
    Resource temp = new Resource("temp");
    try {
        try {
            if (r instanceof DatabaseResource) {
                DatabaseResource dr = (DatabaseResource)r;
                dr.query();
                return "Database operation complete";
            } else {
                throw new CleanupException("Invalid resource type");
            }
        } finally {
            temp.close();
            print("Inner cleanup done");
        }
    } catch (CleanupException e) {
        print("Caught exception: " + e.getMessage());
        return "Operation failed";
    } finally {
        r.close();
        print("Outer cleanup done");
    }
}

// Test 4: Array of resources with partial failure
function processResourceArray(Resource[] resources): string {
    int processed = 0;
    int failed = 0;

    try {
        int i = 0;
        while (i < 3) {
            try {
                if (resources[i] instanceof FileResource) {
                    FileResource fr = (FileResource)resources[i];
                    fr.read();
                    processed = processed + 1;
                } else if (resources[i] instanceof DatabaseResource) {
                    DatabaseResource dr = (DatabaseResource)resources[i];
                    dr.query();
                    processed = processed + 1;
                } else {
                    throw new CleanupException("Unknown resource type");
                }
            } catch (CleanupException e) {
                print("Failed at index " + i + ": " + e.getMessage());
                failed = failed + 1;
            } finally {
                resources[i].close();
            }
            i = i + 1;
        }
        return "Processed: " + processed + ", Failed: " + failed;
    } finally {
        print("All resources cleaned up");
    }
}

// Test 5: Cast failure with exception chaining
function chainedCleanupException(Resource r): string {
    try {
        try {
            if (r instanceof FileResource) {
                FileResource fr = (FileResource)r;
                fr.read();
                throw new CleanupException("Simulated read error");
            } else {
                throw new CleanupException("Not a file resource");
            }
        } catch (CleanupException e) {
            print("Inner exception: " + e.getMessage());
            throw new CleanupException("Re-throwing after cleanup attempt");
        } finally {
            r.close();
        }
    } catch (CleanupException e) {
        return "Outer catch: " + e.getMessage();
    }
}

// Run all tests
print("=== Test 1: Cleanup after failed cast ===");
Resource r1 = new FileResource("file1", "/path/to/file");
print(processCastWithCleanup(r1));

print("\n");
Resource r2 = new DatabaseResource("db1", "localhost:5432");
print(processCastWithCleanup(r2));

print("\n=== Test 2: Multiple resource cleanup ===");
Resource r3 = new FileResource("file2", "/data/log.txt");
Resource r4 = new DatabaseResource("db2", "server:3306");
print(processMultipleResources(r3, r4));

print("\n");
Resource r5 = new DatabaseResource("db3", "cloud:1433");
Resource r6 = new FileResource("file3", "/tmp/data.txt");
print(processMultipleResources(r5, r6));

print("\n=== Test 3: Nested cleanup ===");
Resource r7 = new DatabaseResource("db4", "remote:5000");
print(nestedCastCleanup(r7));

print("\n");
Resource r8 = new FileResource("file4", "/var/config.ini");
print(nestedCastCleanup(r8));

print("\n=== Test 4: Array cleanup with partial failure ===");
Resource[] arr = new Resource[3];
arr[0] = new FileResource("file5", "/home/user/doc.txt");
arr[1] = new DatabaseResource("db5", "local:8080");
arr[2] = new Resource("base");
print(processResourceArray(arr));

print("\n=== Test 5: Chained cleanup exceptions ===");
Resource r9 = new FileResource("file6", "/etc/passwd");
print(chainedCleanupException(r9));

print("\n");
Resource r10 = new DatabaseResource("db6", "backup:9999");
print(chainedCleanupException(r10));

print("\nAll cleanup after failed cast tests completed");
