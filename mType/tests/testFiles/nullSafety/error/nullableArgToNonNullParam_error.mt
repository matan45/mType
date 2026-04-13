// Test: Passing a nullable variable to a non-nullable parameter should fail at compile time

class Item {
    public int id;
    constructor(int id) {
        this.id = id;
    }
}

function process(Item item): void {
    print(item.id);
}

Item? maybe = new Item(1);
process(maybe);
print("Should not reach here");
