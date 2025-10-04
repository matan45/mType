// Test: Protected field access violation from external context
class Item {
    protected int itemID;

    constructor(int id) {
        itemID = id;
    }

    public function getID(): int {
        return itemID;
    }
}

Item item = new Item(42);
print(item.itemID);  // ERROR: Cannot access protected field 'itemID'
