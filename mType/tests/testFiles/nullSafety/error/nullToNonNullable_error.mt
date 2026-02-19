// Test: Assigning null to non-nullable type should fail at compile time

class Item {
    public int id;
    constructor(int id) {
        this.id = id;
    }
}

// This should cause a compile error: null assigned to non-nullable type
Item x = null;
print("Should not reach here");
