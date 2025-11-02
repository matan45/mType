// Test: Generic interface with Promise<T> methods
// @Script

interface AsyncRepository<T> {
    async save(item: T) : Promise<Void>;
    async findById(id: Int) : Promise<T>;
    async findAll() : Promise<T[]>;
}

class Item {
    private id: Int;
    private name: String;

    constructor(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    getId() : Int {
        return this.id;
    }

    getName() : String {
        return this.name;
    }
}

class ItemRepository : AsyncRepository<Item> {
    private items: Item[];

    constructor() {
        this.items = [];
    }

    async save(item: Item) : Promise<Void> {
        await delay(10);
        this.items.push(item);
        print("Saved item: " + item.getName());
    }

    async findById(id: Int) : Promise<Item> {
        await delay(10);
        let i: Int = 0;
        while (i < this.items.length()) {
            if (this.items[i].getId() == id) {
                return this.items[i];
            }
            i = i + 1;
        }
        return this.items[0]; // Default
    }

    async findAll() : Promise<Item[]> {
        await delay(10);
        return this.items;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let repo: AsyncRepository<Item> = new ItemRepository();

    let item1 = new Item(1, "Item A");
    let item2 = new Item(2, "Item B");

    await repo.save(item1);
    await repo.save(item2);

    let found = await repo.findById(2);
    print("Found: " + found.getName());
    assert(found.getName() == "Item B", "Should find correct item");

    let all = await repo.findAll();
    print("Total items: " + all.length().toString());
    assert(all.length() == 2, "Should have 2 items");
}
