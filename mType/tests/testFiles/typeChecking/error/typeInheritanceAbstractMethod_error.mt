// Test: Missing abstract method implementation
// Expected: Error - concrete class must implement all abstract methods from parent

abstract class Renderer {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    // Abstract methods that must be implemented
    abstract function render(): void;
    abstract function clear(): void;
    abstract function setColor(string color): void;
}

// ERROR: This concrete class does not implement all abstract methods
// Missing: clear() and setColor(string)
class SimpleRenderer extends Renderer {
    public constructor(string name) : super(name) {
    }

    // Only implements render(), but missing clear() and setColor()
    public function render(): void {
        print("Rendering with " + this.name);
    }

    // Missing implementations:
    // - clear(): void
    // - setColor(string color): void
}

// This should fail during class registration
SimpleRenderer renderer = new SimpleRenderer("Simple");
renderer.render();
