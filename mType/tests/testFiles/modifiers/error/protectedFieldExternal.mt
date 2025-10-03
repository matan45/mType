// Test: Protected field access violation from external context
class Item {
    protected int itemID;

    public Item(int id) {
        itemID = id;
    }

    public int getID() {
        return itemID;
    }
}

Item item = new Item(42);
print(item.itemID);  // ERROR: Cannot access protected field 'itemID'
