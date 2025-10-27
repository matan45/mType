// Test: Multiple annotations on class and methods
// Expected: Pass - both @Script and @Override work together

interface Updatable {
    function update(): void;
}

@Script
class Component implements Updatable {
    string name;

    constructor(string componentName) {
        name = componentName;
    }

    @Override
    public function update(): void {
        print("Updating component");
    }
}

@Script
class GameObject {
    public function init(): void {
        print("GameObject initialized");
    }
}

@Script
class Entity extends GameObject implements Updatable {
    @Override
    public function init(): void {
        print("Entity initialized");
    }

    @Override
    public function update(): void {
        print("Entity updated");
    }
}

// Test execution
Entity entity = new Entity();
entity.init();
entity.update();
