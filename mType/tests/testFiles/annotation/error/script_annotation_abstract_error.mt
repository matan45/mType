// Error: @Script on abstract class

@Script
abstract class Entity {
    int x;

    constructor() {
        x = 0;
    }

    function update(float dt): void {
        x = x + 1;
    }

    abstract function move(): void;
}
