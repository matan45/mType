// Test concurrent-like modifications across namespaces and classes
int globalCounter = 0;
string sharedMessage = "Initial";

class Counter {
    int localCount;
    
    constructor() {
        localCount = 0;
    }
    
    function increment(): int {
        localCount = localCount + 1;
        globalCounter = globalCounter + 1;
        return localCount;
    }
    
    function getGlobalCount(): int {
        return globalCounter;
    }
}

function batchOperation(): string {
    string result = "";
    
    for (int i = 0; i < 3; i++) {
        Counter counter = new Counter();
        
        for (int j = 0; j < 2; j++) {
            int local = counter.increment();
            int global = counter.getGlobalCount();
            result = result + toString(local) + ":" + toString(global) + " ";
        }
    }
    
    return result;
}

function modifySharedState(): string {
    sharedMessage = sharedMessage + "_modified";
    return sharedMessage;
}

// Test concurrent-like operations
string batchResult = batchOperation();
string modifiedMessage = modifySharedState();

print(batchResult);
print(modifiedMessage);

// Test multiple counters affecting shared state
Counter counter1 = new Counter();
Counter counter2 = new Counter();

int c1_1 = counter1.increment();
int c2_1 = counter2.increment();
int c1_2 = counter1.increment();

print(c1_1);
print(c2_1);
print(c1_2);
print(counter1.getGlobalCount());
print(counter2.getGlobalCount());