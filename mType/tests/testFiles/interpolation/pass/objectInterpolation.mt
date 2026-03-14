// Test string interpolation with objects
class Item {
    private string name;
    private int value;

    constructor(string name, int value) {
        this.name = name;
        this.value = value;
    }

    public function getName(): string {
        return this.name;
    }

    public function getValue(): int {
        return this.value;
    }
}

Item item = new Item("Sword", 100);
print($"item name: {item.getName()}, value: {item.getValue()}");
