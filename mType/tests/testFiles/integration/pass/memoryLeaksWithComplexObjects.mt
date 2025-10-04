// Test memory management with complex object relationships
class Resource {
    string name;
    int id;
    static int totalResources = 0;

    public constructor(string resourceName, int resourceId) {
        name = resourceName;
        id = resourceId;
        totalResources = totalResources + 1;
    }

    public function getInfo(): string {
        return "Resource " + name + " [" + id + "]";
    }

    public static function getTotalResources(): int {
        return totalResources;
    }
}

class Manager {
    Resource primaryResource;
    Resource secondaryResource;
    final int MAX_MANAGED_RESOURCES = 2;

    public constructor(string primary, string secondary) {
        primaryResource = new Resource(primary, 1);
        secondaryResource = new Resource(secondary, 2);
    }

    public function processResources(): string {
        string result = "";
        result = result + primaryResource.getInfo() + " | ";
        result = result + secondaryResource.getInfo();
        return result;
    }

    public function swapResources(): void {
        Resource temp = primaryResource;
        primaryResource = secondaryResource;
        secondaryResource = temp;
    }
}

function createComplexObjectGraph(): string {
    string results = "";
    final int NUM_MANAGERS = 5;
    
    for (int i = 0; i < NUM_MANAGERS; i++) {
        Manager mgr = new Manager("Primary" + i, "Secondary" + i);
        
        if (i % 2 == 0) {
            mgr.swapResources();
        }
        
        string info = mgr.processResources();
        results = results + "Manager " + i + ": " + info + " | ";
    }
    
    return results;
}

// Test complex object creation and relationships
string objectResults = createComplexObjectGraph();
print(objectResults);
print(Resource::getTotalResources());