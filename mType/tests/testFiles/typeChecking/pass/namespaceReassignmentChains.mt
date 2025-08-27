namespace storage {
    class Container {
        string data;
        constructor(string d) {
            data = d;
        }
        
        function duplicate(): Container {
            return new Container(data);
        }
    }
    
    Container globalContainer = new Container("Global");
    
    function getContainer(): Container {
        return new Container("Function");
    }
}

namespace processing {
    class Processor {
        string status;
        constructor(string s) {
            status = s;
        }
    }
    
    Processor processor1 = new Processor("Ready");
    Processor processor2 = new Processor("Waiting");
}

using namespace storage;
using namespace processing;

// Test reassignment chains across namespaces
Container c1 = new Container("C1");
Container c2 = new Container("C2");
Container c3 = new Container("C3");

c1 = c2 = storage::globalContainer;
print("Namespace chain: " + c1.data);
print("Namespace chain 2: " + c2.data);

// Test chain with qualified and unqualified access
c1 = c2 = c3 = storage::getContainer();
print("Function chain: " + c1.data);

// Test mixed namespace reassignment
Container mixed1 = new Container("Mixed1");
storage::Container mixed2 = new storage::Container("Mixed2");
mixed1 = mixed2;
print("Mixed namespace: " + mixed1.data);

// Test processor reassignment chains
processing::Processor p1 = new processing::Processor("P1");
processing::Processor p2 = new processing::Processor("P2");
p1 = p2 = processing::processor1;
print("Processor chain: " + p1.status);

// Test cross-namespace with different types (should work if compatible)
Container container = new Container("Test");
storage::Container storageContainer = new storage::Container("storage");
container = storageContainer;  // Should work - same type across namespaces
print("Cross-namespace: " + container.data);