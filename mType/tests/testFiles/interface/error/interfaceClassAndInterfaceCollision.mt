// Test class extends + implements with collision
// @Throw

class Base {
    func process(): String {
        return "Base processing";
    }
}

interface Processor {
    func process(): Int;  // Different return type from Base.process()
}

// Error: Cannot implement Processor because process() has conflicting signature
class Implementation extends Base implements Processor {
    // Cannot satisfy both Base.process():String and Processor.process():Int
    func process(): Int {
        return 42;
    }
}

var impl = new Implementation();
print(impl.process());
