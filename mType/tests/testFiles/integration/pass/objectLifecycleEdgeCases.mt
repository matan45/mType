// Test edge cases in object lifecycle management
class LifecycleCounter {
    public static int instanceCount = 0;
    public static int maxInstanceCount = 0;
    public int instanceId;
    public string phase;

    public constructor() {
        instanceCount = instanceCount + 1;
        instanceId = instanceCount;
        phase = "Created";

        if (instanceCount > maxInstanceCount) {
            maxInstanceCount = instanceCount;
        }
    }

    public function getId(): int {
        return instanceId;
    }

    public function getPhase(): string {
        return phase;
    }

    public function setPhase(string newPhase): void {
        phase = newPhase;
    }

    public static function getCurrentCount(): int {
        return instanceCount;
    }

    public static function getMaxCount(): int {
        return maxInstanceCount;
    }

    public static function resetCounters(): void {
        instanceCount = 0;
        maxInstanceCount = 0;
    }
}
function testRapidObjectCreationDestruction(): void {
    LifecycleCounter::resetCounters();
    print("=== Rapid Object Creation/Destruction Test ===");
    
    // Create many objects rapidly
    for (int i = 0; i < 100; i++) {
        LifecycleCounter temp = new LifecycleCounter();
        temp.setPhase("Processing");
        
        if (i % 10 == 0) {
            print("Iteration " + i + ": Current count = " +
                  LifecycleCounter::getCurrentCount());
        }
        
        // temp goes out of scope and should be destroyed
    }
    
    print("After rapid creation: Max count = " + LifecycleCounter::getMaxCount() +
          ", Current count = " + LifecycleCounter::getCurrentCount());
}

function testObjectArrayLifecycle(): void {
    LifecycleCounter::resetCounters();
    print("=== Object Array Lifecycle Test ===");
    
    // Note: Arrays are not implemented in the current system
    // This test focuses on multiple object variables
    LifecycleCounter obj1 = new LifecycleCounter();
    LifecycleCounter obj2 = new LifecycleCounter();
    LifecycleCounter obj3 = new LifecycleCounter();
    
    print("Created 3 objects: " + LifecycleCounter::getCurrentCount());
    
    obj1.setPhase("Active");
    obj2.setPhase("Inactive");
    obj3.setPhase("Pending");
    
    // Reassign some objects
    obj1 = obj2; // Original obj1 should be destroyed
    obj3 = null; // obj3 should be destroyed
    
    print("After reassignments: " + LifecycleCounter::getCurrentCount());
    
    // Create new object and assign
    obj1 = new LifecycleCounter();
    obj1.setPhase("NewActive");
    
    print("After new creation: " + LifecycleCounter::getCurrentCount());
}

function testFunctionParameterLifecycle(): void {
    LifecycleCounter::resetCounters();
    print("=== Function Parameter Lifecycle Test ===");
    
    function processObject(LifecycleCounter obj): string {
        obj.setPhase("Processing");
        return "Processed object " + obj.getId() + " in phase: " + obj.getPhase();
    }
    
    LifecycleCounter mainObj = new LifecycleCounter();
    print("Before function call: " + LifecycleCounter::getCurrentCount());
    
    string result = processObject(mainObj);
    print(result);
    
    print("After function call: " + LifecycleCounter::getCurrentCount());
    
    // Test with temporary object
    string result2 = processObject(new LifecycleCounter());
    print(result2);
    
    print("After temporary object: " + LifecycleCounter::getCurrentCount());
}

function testRecursiveObjectCreation(): void {
    LifecycleCounter::resetCounters();
    print("=== Recursive Object Creation Test ===");
    
    function createObjectRecursively(int depth): int {
        if (depth <= 0) {
            return LifecycleCounter::getCurrentCount();
        }
        
        LifecycleCounter obj = new LifecycleCounter();
        obj.setPhase("Depth" + depth);
        
        int result = createObjectRecursively(depth - 1);
        
        // obj should be destroyed when function returns
        return result;
    }
    
    print("Starting recursive creation...");
    int maxCount = createObjectRecursively(10);
    
    print("Max objects during recursion: " + maxCount);
    print("Objects after recursion: " + LifecycleCounter::getCurrentCount());
}

function testObjectSharingScenario(): void {
    LifecycleCounter::resetCounters();
    print("=== Object Sharing Scenario Test ===");
    
    LifecycleCounter shared = new LifecycleCounter();
    shared.setPhase("Shared");
    
    LifecycleCounter ref1 = shared;
    LifecycleCounter ref2 = shared;
    LifecycleCounter ref3 = shared;
    
    print("After sharing: " + LifecycleCounter::getCurrentCount());
    print("Shared object phase: " + shared.getPhase());
    
    // Modify through one reference
    ref1.setPhase("Modified");
    
    print("After modification through ref1:");
    print("  shared.phase: " + shared.getPhase());
    print("  ref2.phase: " + ref2.getPhase());
    print("  ref3.phase: " + ref3.getPhase());
    
    // Clear references
    ref1 = null;
    ref2 = null;
    ref3 = null;
    
    print("After clearing references: " + LifecycleCounter::getCurrentCount());
    print("Original object still accessible: " + shared.getPhase());
}

// Run all lifecycle tests
testRapidObjectCreationDestruction();
testObjectArrayLifecycle();
testFunctionParameterLifecycle();
testRecursiveObjectCreation();
testObjectSharingScenario();