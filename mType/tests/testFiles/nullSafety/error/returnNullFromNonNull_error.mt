// Test: Returning null from a function with non-nullable return type should fail at compile time

class Item {
    public int id;
    constructor(int id) {
        this.id = id;
    }
}

function getItem(): Item {
    return null;
}

Item i = getItem();
print("Should not reach here");
