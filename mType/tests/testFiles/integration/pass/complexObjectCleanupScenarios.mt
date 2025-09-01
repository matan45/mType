// Test complex cleanup scenarios with circular references and resource management

class ResourceManager {
    string resourceName;
    int resourceCount;
    static int totalResourcesAllocated = 0;
    static int totalResourcesReleased = 0;
    
    constructor(string name, int count) {
        resourceName = name;
        resourceCount = count;
        totalResourcesAllocated = totalResourcesAllocated + count;
    }
    
    function releaseResource(): void {
        totalResourcesReleased = totalResourcesReleased + resourceCount;
        resourceCount = 0;
    }
    
    function getResourceInfo(): string {
        return resourceName + ": " + toString(resourceCount) + " units";
    }
    
    static function getResourceStats(): string {
        return "Allocated: " + toString(totalResourcesAllocated) + 
               ", Released: " + toString(totalResourcesReleased);
    }
    
    static function resetStats(): void {
        totalResourcesAllocated = 0;
        totalResourcesReleased = 0;
    }
}
class Node {
    string data;
    Node next;
    ResourceManager resource;
    
    constructor(string nodeData) {
        data = nodeData;
        next = null;
        resource = new ResourceManager(nodeData + "_Resource", 10);
    }
    
    function setNext(Node nextNode): void {
        next = nextNode;
    }
    
    function getData(): string {
        return data;
    }
    
    function getNext(): Node {
        return next;
    }
    
    function cleanup(): void {
        if (resource != null) {
            resource.releaseResource();
        }
    }
}
function testCircularReferenceCleanup(): void {
    ResourceManager::resetStats();
    print("=== Circular Reference Cleanup Test ===");
    
    // Create circular reference chain
    Node node1 = new Node("Node1");
    Node node2 = new Node("Node2");
    Node node3 = new Node("Node3");
    
    node1.setNext(node2);
    node2.setNext(node3);
    node3.setNext(node1); // Circular reference
    
    print("Before cleanup: " + ResourceManager::getResourceStats());
    
    // Explicit cleanup
    node1.cleanup();
    node2.cleanup();
    node3.cleanup();
    
    print("After explicit cleanup: " + ResourceManager::getResourceStats());
    
    // Breaking circular references
    node1.setNext(null);
    node2.setNext(null);
    node3.setNext(null);
    
    print("Circular references broken");
}
function testNestedObjectCleanup(): void {
    ResourceManager::resetStats();
    print("=== Nested Object Cleanup Test ===");
    
    // Create nested object structure
    {
        ResourceManager outerResource = new ResourceManager("Outer", 100);
        {
            ResourceManager innerResource1 = new ResourceManager("Inner1", 50);
            ResourceManager innerResource2 = new ResourceManager("Inner2", 25);
            
            print("Inner scope resources: " + innerResource1.getResourceInfo() + 
                  ", " + innerResource2.getResourceInfo());
            
            innerResource1.releaseResource();
            // innerResource2 not explicitly released
        }
        
        print("Back to outer scope: " + outerResource.getResourceInfo());
        outerResource.releaseResource();
    }
    
    print("Final resource stats: " + ResourceManager::getResourceStats());
}
function testObjectReassignmentCleanup(): void {
    ResourceManager::resetStats();
    print("=== Object Reassignment Cleanup Test ===");
    
    ResourceManager manager = new ResourceManager("Original", 75);
    print("Original manager: " + manager.getResourceInfo());
    
    // Reassign to new object (old object should be cleaned up)
    manager = new ResourceManager("Replacement", 150);
    print("After reassignment: " + manager.getResourceInfo());
    
    // Assign to null
    manager = null;
    print("After null assignment - Stats: " + ResourceManager::getResourceStats());
}
function testExceptionSafeCleanup(): void {
    ResourceManager::resetStats();
    print("=== Exception Safe Cleanup Test ===");
    
    ResourceManager safeResource = new ResourceManager("SafeResource", 200);
    
    {
        ResourceManager riskyResource = new ResourceManager("RiskyResource", 300);
        print("Before potential exception: " + ResourceManager::getResourceStats());
        
        // Simulate error condition (division by zero would trigger exception)
        // For this test, we'll just do normal cleanup
        riskyResource.releaseResource();
        safeResource.releaseResource();
    }
    
    print("After exception handling: " + ResourceManager::getResourceStats());
}

// Run all cleanup tests
testCircularReferenceCleanup();
testNestedObjectCleanup();
testObjectReassignmentCleanup();
testExceptionSafeCleanup();