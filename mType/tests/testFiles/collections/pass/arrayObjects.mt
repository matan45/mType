// Array with objects test

class Item {
    string name;
    float price;
    
    constructor(string n, float p) {
        name = n;
        price = p;
    }
    
    function getName(): string {
        return name;
    }
    
    function getPrice(): float {
        return price;
    }
}

// Create array of items
Array<Item> items = new Array<Item>();

// Add items
Item apple = new Item("Apple", 1.50);
Item banana = new Item("Banana", 0.75);
Item cherry = new Item("Cherry", 2.25);

items.add(apple);
items.add(banana);
items.add(cherry);

print(items.size()); // 3

// Access items
Item firstItem = items[0];
print(firstItem.getName());  // "Apple"
print(firstItem.getPrice()); // 1.5

Item secondItem = items.get(1);
print(secondItem.getName());  // "Banana"
print(secondItem.getPrice()); // 0.75

// Replace an item
Item orange = new Item("Orange", 1.25);
items.set(2, orange);

Item thirdItem = items[2];
print(thirdItem.getName());  // "Orange"
print(thirdItem.getPrice()); // 1.25

print("Array objects test passed");