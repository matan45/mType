// Test: Generic interface with Promise<T> methods
// @Script

interface AsyncRepository<T> {
    function async save(T item) : Promise<void>;
    function async findById(Int id) : Promise<T>;
    function async findAll() : Promise<T[]>;
}

class Item {
    private Int id;
    private String name;

    constructor(Int id, String name) {
        this.id = id;
        this.name = name;
    }

    public function getId() : Int {
        return this.id;
    }

    public function getName() : String {
        return this.name;
    }
}

class ItemRepository implements AsyncRepository<Item> {
    private Item[] items;

    constructor() {
        this.items = [];
    }

    public function async save(Item item) : Promise<void> {
        await delay(10);
        this.items.push(item);
        print("Saved item: " + item.getName());
    }

    public function async findById(Int id) : Promise<Item> {
        await delay(10);
        Int i = 0;
        while (i < this.items.length()) {
            if (this.items[i].getId() == id) {
                return this.items[i];
            }
            i = i + 1;
        }
        return this.items[0]; // Default
    }

    public function async findAll() : Promise<Item[]> {
        await delay(10);
        return this.items;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    AsyncRepository<Item> repo = new ItemRepository();

    Item item1 = new Item(1, "Item A");
    Item item2 = new Item(2, "Item B");

    await repo.save(item1);
    await repo.save(item2);

    Item found = await repo.findById(2);
    print("Found: " + found.getName());
    assert(found.getName() == "Item B", "Should find correct item");

    Item[] all = await repo.findAll();
    print("Total items: " + all.length().toString());
    assert(all.length() == 2, "Should have 2 items");
}
