// Test: Multiple annotations on class and methods
// Expected: Pass - both @Script and @Override work together

interface Updatable {
    function update(float dt): void;
}

@Script
class Component implements Updatable {
    string name;
	
	constructor(){}

    constructor(string componentName) {
        name = componentName;
    }

    @Override
    public function update(float dt): void {
        print("Updating component");
    }
}

@Script
class GameObject {

	constructor(){}
    public function init(): void {
        print("GameObject initialized");
    }
	
	public function update(float dt): void {
        print("Updating component");
    }
}

@Script
class Entity extends GameObject implements Updatable {
	
	constructor(){}

    @Override
    public function init(): void {
        print("Entity initialized");
    }

    @Override
    public function update(float dt): void {
        print("Entity updated");
    }
}

// Test execution
Entity entity = new Entity();
entity.init();
entity.update(0.004);
