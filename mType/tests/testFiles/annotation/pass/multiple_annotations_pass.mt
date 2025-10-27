// Test: Multiple annotations on class and methods
// Expected: Pass - both @Script and @Override work together

interface Updatable {
    void update();
}

@Script
class Component implements Updatable {
    String name;

    Component(String componentName) {
        name = componentName;
    }

    @Override
    void update() {
        print("Updating component");
    }
}

@Script
class GameObject {
    void init() {
        print("GameObject initialized");
    }
}

@Script
class Entity extends GameObject implements Updatable {
    @Override
    void init() {
        print("Entity initialized");
    }

    @Override
    void update() {
        print("Entity updated");
    }
}

// Test execution
Entity entity = new Entity();
entity.init();
entity.update();
