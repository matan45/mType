// All Three Test 1: Generic class with array fields
@Script

class Repository<T> {
    field items: T[];
    field capacity: Int;
    field size: Int;
    field name: String;

    constructor(repoName: String, cap: Int) {
        this.name = repoName;
        this.capacity = cap;
        this.items = T[cap];
        this.size = 0;
    }

    fun add(item: T): Bool {
        if (this.size >= this.capacity) {
            return false;
        }
        this.items[this.size] = item;
        this.size = this.size + 1;
        return true;
    }

    fun get(index: Int): T {
        return this.items[index];
    }

    fun getSize(): Int {
        return this.size;
    }

    fun getName(): String {
        return this.name;
    }

    fun findFirst(target: T): Int {
        let i: Int = 0;
        while (i < this.size) {
            if (this.items[i] == target) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    fun toArray(): T[] {
        let result: T[] = T[this.size];
        let i: Int = 0;
        while (i < this.size) {
            result[i] = this.items[i];
            i = i + 1;
        }
        return result;
    }
}

class Entity {
    field id: Int;
    field name: String;

    constructor(identifier: Int, entityName: String) {
        this.id = identifier;
        this.name = entityName;
    }

    fun getId(): Int {
        return this.id;
    }

    fun getName(): String {
        return this.name;
    }
}

print("Repository<Int> test:");
let intRepo: Repository<Int> = Repository<Int>("Numbers", 5);
print(intRepo.getName());

print(intRepo.add(10));
print(intRepo.add(20));
print(intRepo.add(30));

print("Size:");
print(intRepo.getSize());

print("Contents:");
let i: Int = 0;
while (i < intRepo.getSize()) {
    print(intRepo.get(i));
    i = i + 1;
}

print("Find 20:");
print(intRepo.findFirst(20));

print("Repository<String> test:");
let strRepo: Repository<String> = Repository<String>("Words", 4);
strRepo.add("alpha");
strRepo.add("beta");
strRepo.add("gamma");

let strArray: String[] = strRepo.toArray();
print("Exported array:");
i = 0;
while (i < 3) {
    print(strArray[i]);
    i = i + 1;
}

print("Repository<Entity> test:");
let entityRepo: Repository<Entity> = Repository<Entity>("Entities", 3);
entityRepo.add(Entity(1, "First"));
entityRepo.add(Entity(2, "Second"));
entityRepo.add(Entity(3, "Third"));

print("Entity repository:");
i = 0;
while (i < entityRepo.getSize()) {
    let e: Entity = entityRepo.get(i);
    print(e.getId());
    print(e.getName());
    i = i + 1;
}
