// Test: Passing null literal to a non-nullable parameter should fail at compile time

class Item {
    public int id;
    constructor(int id) {
        this.id = id;
    }
}

function process(Item item): void {
    print(item.id);
}

process(null);
print("Should not reach here");
