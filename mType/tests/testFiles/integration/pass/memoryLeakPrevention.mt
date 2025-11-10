// Test memory leak prevention in various scenarios
class Container {
            LeakDetector item1;
            LeakDetector item2;
            string containerName;

            constructor(string name) {
                containerName = name;
                item1 = new LeakDetector(name + "_Item1");
                item2 = new LeakDetector(name + "_Item2");
            }

            public function getContainerInfo(): string {
                return containerName + ": [" + item1.getName() + ", " + item2.getName() + "]";
            }

            public function swapItems(): void {
                LeakDetector temp = item1;
                item1 = item2;
                item2 = temp;
            }
        }

class LeakDetector {
        string name;
        static int totalAllocations = 0;
        static int peakAllocations = 0;
        static int currentAllocations = 0;
        
        constructor(string objectName) {
            name = objectName;
            totalAllocations = totalAllocations + 1;
            currentAllocations = currentAllocations + 1;
            
            if (currentAllocations > peakAllocations) {
                peakAllocations = currentAllocations;
            }
        }
        
        public function getName(): string {
            return name;
        }
        
        public static function getAllocationStats(): string {
            return "Total: " + totalAllocations +
                   ", Current: " + currentAllocations +
                   ", Peak: " + peakAllocations;
        }

        public static function resetStats(): void {
            totalAllocations = 0;
            peakAllocations = 0;
            currentAllocations = 0;
        }
        
        static function getCurrentAllocations(): int {
            return currentAllocations;
        }
    }
    
    public function testLoopObjectCreation(): void {
        LeakDetector::resetStats();
        print("=== Loop Object Creation Test ===");
        
        // Test that objects created in loops are properly cleaned up
        for (int i = 0; i < 50; i++) {
            LeakDetector obj = new LeakDetector("LoopObject" + i);
            
            // Do some work with the object
            string name = obj.getName();
            
            if (i % 10 == 0) {
                print("Loop " + i + ": " + LeakDetector::getAllocationStats());
            }
            
            // obj should be cleaned up at end of loop iteration
        }
        
        print("After loop: " + LeakDetector::getAllocationStats());
    }
    
    public function testConditionalObjectCreation(): void {
        LeakDetector::resetStats();
        print("=== Conditional Object Creation Test ===");
        
        for (int i = 0; i < 20; i++) {
            if (i % 2 == 0) {
                LeakDetector evenObj = new LeakDetector("Even" + i);
                print("Created even object: " + evenObj.getName());
            } else {
                LeakDetector oddObj = new LeakDetector("Odd" + i);
                print("Created odd object: " + oddObj.getName());
            }
            
            // Objects should be cleaned up regardless of which branch was taken
        }
        
        print("After conditional creation: " + LeakDetector::getAllocationStats());
    }
    public function createAndProcess(string baseName): string {
            LeakDetector inner = new LeakDetector(baseName + "_Inner");
            return inner.getName() + "_Processed";
        }
        
        public function processMultiple(int count): string {
            string result = "";
            for (int i = 0; i < count; i++) {
                string processed = createAndProcess("Batch" + i);
                result = result + processed + " | ";
            }
            return result;
        }
    public function testNestedFunctionCalls(): void {
        LeakDetector::resetStats();
        print("=== Nested Function Calls Test ===");
        
        
        
        string batchResult = processMultiple(10);
        print("Batch processing result length: " + strLength(batchResult));
        print("After batch processing: " + LeakDetector::getAllocationStats());
    }
	
	public function createObject(string name): LeakDetector {
            LeakDetector newObj = new LeakDetector(name);
            return newObj;
        }
    
    public function testObjectReturnFromFunction(): void {
        LeakDetector::resetStats();
        print("=== Object Return From Function Test ===");
        
        
        
        LeakDetector obj1 = createObject("Returned1");
        LeakDetector obj2 = createObject("Returned2");
        LeakDetector obj3 = createObject("Returned3");
        
        print("After creating returned objects: " + LeakDetector::getAllocationStats());
        
        print("Objects: " + obj1.getName() + ", " + obj2.getName() + ", " + obj3.getName());
        
        // Reassign one object
        obj2 = createObject("NewReturned2");
        
        print("After reassignment: " + LeakDetector::getAllocationStats());
    }
	
	
    
    public function testComplexObjectGraph(): void {
        LeakDetector::resetStats();
        print("=== Complex Object Graph Test ===");
        
        
        Container container1 = new Container("Container1");
        Container container2 = new Container("Container2");
        
        print("After container creation: " + LeakDetector::getAllocationStats());
        print("Container1: " + container1.getContainerInfo());
        print("Container2: " + container2.getContainerInfo());
        
        container1.swapItems();
        container2.swapItems();
        
        print("After swapping: " + container1.getContainerInfo());
        
        // Replace containers
        container1 = new Container("NewContainer1");
        
        print("After replacement: " + LeakDetector::getAllocationStats());
    }
    
    public function testMemoryPressureScenario(): void {
        LeakDetector::resetStats();
        print("=== Memory Pressure Scenario Test ===");
        
        // Simulate high memory pressure scenario
        final int BATCH_SIZE = 25;
        final int NUM_BATCHES = 4;
        
        for (int batch = 0; batch < NUM_BATCHES; batch++) {
            print("Starting batch " + (batch + 1) + "/" + NUM_BATCHES);
            
            for (int i = 0; i < BATCH_SIZE; i++) {
                LeakDetector obj = new LeakDetector("Batch" + batch + "_Item" + i);
                
                // Simulate some work
                string name = obj.getName();
            }
            
            print("After batch " + (batch + 1) + ": " + LeakDetector::getAllocationStats());
            
            // Potentially trigger cleanup between batches
            // In a real implementation, this might involve garbage collection
        }
        
        print("Final memory pressure test stats: " + LeakDetector::getAllocationStats());
    }

// Run all memory leak prevention tests
testLoopObjectCreation();
testConditionalObjectCreation();
testNestedFunctionCalls();
testObjectReturnFromFunction();
testComplexObjectGraph();
testMemoryPressureScenario();