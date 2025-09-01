// Memory usage test with deep nesting
class NestedContainer {
    int level;
    string data;
    
    constructor(int containerLevel) {
        level = containerLevel;
        data = "Level" + toString(level);
    }
    
    function processDeepNesting(int depth): int {
        if (depth <= 0) {
            return level;
        }
        
        NestedContainer child = new NestedContainer(level + 1);
        return level + child.processDeepNesting(depth - 1);
    }
}

function memoryStressTest(): int {
    int totalResult = 0;
    final int ITERATIONS = 10;
    
    for (int i = 0; i < ITERATIONS; i++) {
        NestedContainer container = new NestedContainer(i);
        int result = container.processDeepNesting(5);
        totalResult = totalResult + result;
    }
    
    return totalResult;
}

// Run memory usage test
int memoryResult = memoryStressTest();
print(memoryResult);