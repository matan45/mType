// Test basic object destruction order with scope-based cleanup
class DestructionTracker {
    string name;
    static string destructionLog = "";

    public constructor(string objectName) {
        name = objectName;
        destructionLog = destructionLog + "Created: " + name + " | ";
    }

    public function getName(): string {
        return name;
    }

    public static function getDestructionLog(): string {
        return destructionLog;
    }

    public static function clearLog(): void {
        destructionLog = "";
    }
}

function testScopeBasedDestruction(): void {
    DestructionTracker::clearLog();
    
    // Create objects in nested scopes
    {
        DestructionTracker obj1 = new DestructionTracker("Object1");
        {
            DestructionTracker obj2 = new DestructionTracker("Object2");
            DestructionTracker obj3 = new DestructionTracker("Object3");
            
            print("In inner scope: " + obj1.getName() + ", " + obj2.getName() + ", " + obj3.getName());
            // obj2 and obj3 should be destroyed when inner scope ends
        }
        print("Back in outer scope: " + obj1.getName());
        // obj1 should be destroyed when outer scope ends
    }
    
    print("Final destruction log: " + DestructionTracker::getDestructionLog());
}

function testSequentialDestruction(): void {
    DestructionTracker::clearLog();
    
    // Create objects sequentially
    DestructionTracker a = new DestructionTracker("A");
    DestructionTracker? b = new DestructionTracker("B");
    DestructionTracker c = new DestructionTracker("C");
    
    // Reassign to trigger potential cleanup
    a = new DestructionTracker("A-New");
    b = null;
    c = new DestructionTracker("C-New");
    
    print("Sequential destruction log: " + DestructionTracker::getDestructionLog());
}

// Run tests
testScopeBasedDestruction();
testSequentialDestruction();