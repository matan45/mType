// Test resource cleanup patterns and RAII-like behavior

class FileHandle {
    string filename;
    bool isOpen;
    static int openHandles = 0;
    static string operationLog = "";

    constructor(string name) {
    filename = name;
    isOpen = true;
    openHandles = openHandles + 1;
    operationLog = operationLog + "OPEN:" + name + " | ";
    }

    public function close(): void {
    if (isOpen) {
        isOpen = false;
        openHandles = openHandles - 1;
        operationLog = operationLog + "CLOSE:" + filename + " | ";
    }
    }
    
    public function isFileOpen(): bool {
    return isOpen;
    }
    
    public function getFilename(): string {
    return filename;
    }
    
    public static function getOpenHandleCount(): int {
    return openHandles;
    }
    
    public static function getOperationLog(): string {
    return operationLog;
    }

    public static function clearLog(): void {
    operationLog = "";
    openHandles = 0;
    }
}
function testRAIIPattern(): void {
    FileHandle::clearLog();
    print("=== RAII Pattern Test ===");
    
    // Test automatic cleanup when objects go out of scope
    {
    FileHandle file1 = new FileHandle("config.txt");
    FileHandle file2 = new FileHandle("data.txt");
    
    print("In scope - Open handles: " + FileHandle::getOpenHandleCount());
    print("File1 open: " + file1.isFileOpen());
    print("File2 open: " + file2.isFileOpen());
    
    // Explicit close for one file
    file1.close();
    print("After explicit close - Open handles: " + FileHandle::getOpenHandleCount());
    
    // file2 should be automatically cleaned up when scope ends
    }
    
    print("After scope - Open handles: " + FileHandle::getOpenHandleCount());
    print("Operations: " + FileHandle::getOperationLog());
}

function transferFile(FileHandle source): FileHandle {
        print("Transferring: " + source.getFilename());
        return source; // Transfer ownership
    }
function testResourceTransferPattern(): void {
    FileHandle::clearLog();
    print("=== Resource Transfer Pattern Test ===");
    
    
    
    FileHandle original = new FileHandle("transfer.txt");
    print("Before transfer - Open handles: " + FileHandle::getOpenHandleCount());
    
    FileHandle transferred = transferFile(original);
    print("After transfer - Open handles: " + FileHandle::getOpenHandleCount());
    
    print("Original filename: " + original.getFilename());
    print("Transferred filename: " + transferred.getFilename());
    print("Same object: " + (original.getFilename() == transferred.getFilename()));
    
    transferred.close();
    print("After explicit close - Open handles: " + FileHandle::getOpenHandleCount());
    print("Operations: " + FileHandle::getOperationLog());
    }
	
	public function riskyOperation(FileHandle file): void {
        print("Starting risky operation on: " + file.getFilename());
        
        // Simulate potential error condition
        // In a real scenario, this might throw an exception
        if (file.getFilename() == "risky.txt") {
        print("Simulating error condition");
        // Would throw exception in real scenario
        }
        
        print("Risky operation completed successfully");
    }
function testExceptionSafeResource(): void {
    FileHandle::clearLog();
    print("=== Exception Safe Resource Test ===");
    
    
    
    FileHandle safeFile = new FileHandle("safe.txt");
    FileHandle riskyFile = new FileHandle("risky.txt");
    
    print("Before risky operations - Open handles: " + FileHandle::getOpenHandleCount());
    
    {
        riskyOperation(safeFile);
        riskyOperation(riskyFile);
    }
    
    print("After operations - Open handles: " + FileHandle::getOpenHandleCount());
    
    // Explicit cleanup
    safeFile.close();
    riskyFile.close();
    
    print("After explicit cleanup - Open handles: " + FileHandle::getOpenHandleCount());
    print("Operations: " + FileHandle::getOperationLog());
    }
	
	class ResourcePool {
        FileHandle resource1;
        FileHandle resource2;
        FileHandle resource3;
        int activeResources;
        
        constructor() {
        resource1 = new FileHandle("pool_resource_1.txt");
        resource2 = new FileHandle("pool_resource_2.txt");
        resource3 = new FileHandle("pool_resource_3.txt");
        activeResources = 3;
        }
        
        public function getAvailableResource(): FileHandle? {
        // Simple round-robin allocation
        if (resource1.isFileOpen()) {
            return resource1;
        } else if (resource2.isFileOpen()) {
            return resource2;
        } else if (resource3.isFileOpen()) {
            return resource3;
        }
        
        // All resources closed, return null
        return null;
        }
        
        public function releaseResource(FileHandle? resource): void {
        if (resource != null) {
            resource.close();
            activeResources = activeResources - 1;
        }
        }
        
        public function getActiveResourceCount(): int {
        return activeResources;
        }
        
        public function cleanup(): void {
        resource1.close();
        resource2.close();
        resource3.close();
        activeResources = 0;
        }
    }
function testResourcePoolPattern(): void {
    FileHandle::clearLog();
    print("=== Resource Pool Pattern Test ===");
    
    
    ResourcePool pool = new ResourcePool();
    print("Pool created - Open handles: " + FileHandle::getOpenHandleCount());
    
    FileHandle acquired1 = pool.getAvailableResource();
    FileHandle acquired2 = pool.getAvailableResource();
    
    if (acquired1 != null) {
        print("Acquired resource: " + acquired1.getFilename());
    }
    if (acquired2 != null) {
        print("Acquired resource: " + acquired2.getFilename());
    }
    
    pool.releaseResource(acquired1);
    print("After releasing one resource - Active: " + pool.getActiveResourceCount());
    
    pool.cleanup();
    print("After pool cleanup - Open handles: " + FileHandle::getOpenHandleCount());
    print("Operations: " + FileHandle::getOperationLog());
    }
	
	public function processWithNestedResources(): void {
        FileHandle outer = new FileHandle("outer.txt");
        
        {
        FileHandle inner1 = new FileHandle("inner1.txt");
        {
            FileHandle inner2 = new FileHandle("inner2.txt");
            FileHandle inner3 = new FileHandle("inner3.txt");
            
            print("Deepest level - Open handles: " + FileHandle::getOpenHandleCount());
            
            inner2.close(); // Explicit close
            // inner3 should be auto-cleaned
        }
        
        print("Middle level - Open handles: " + FileHandle::getOpenHandleCount());
        // inner1 should be auto-cleaned
        }
        
        print("Outer level - Open handles: " + FileHandle::getOpenHandleCount());
        outer.close();
    }
function testNestedResourceCleanup(): void {
    FileHandle::clearLog();
    print("=== Nested Resource Cleanup Test ===");
    
    
    
    processWithNestedResources();
    
    print("After nested processing - Open handles: " + FileHandle::getOpenHandleCount());
    print("Operations: " + FileHandle::getOperationLog());
}

// Run all resource cleanup pattern tests
testRAIIPattern();
testResourceTransferPattern();
testExceptionSafeResource();
testResourcePoolPattern();
testNestedResourceCleanup();