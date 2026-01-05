// Test: Multiple annotations on class and methods
// Expected: Pass - both @Script and @Override work together


@Script
class Component {
    string name;

	constructor(){}

    constructor(string componentName) {
        name = componentName;
    }

   
    public function onUpdate(float deltaTime): void {
        print("Updating component");
    }

    public function onStart(): void {}
    public function onDestroy(): void {}
}

@Script
class GameObject {

	constructor(){}

    public function init(): void {
        print("GameObject initialized");
    }

	public function onUpdate(float deltaTime): void {
        print("Updating component");
    }

    public function onStart(): void {}
    public function onDestroy(): void {}
}

@Script
class Entity extends GameObject implements Updatable {

	constructor(){}

    @Override
    public function init(): void {
        print("Entity initialized");
    }

    @Override
    public function onUpdate(float deltaTime): void {
        print("Entity updated");
    }

    public function onStart(): void {}
    public function onDestroy(): void {}
}

// Test execution
Entity entity = new Entity();
entity.init();
entity.onUpdate(0.004);
