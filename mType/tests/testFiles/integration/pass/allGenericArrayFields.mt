// All Three Test 1: Generic class with array fields
@Script

class Repository<T> {
    T[] items;
    Int capacity;
    Int size;
    String name;

    constructor(repoName: String, cap: Int) {
        this.name = repoName;
        this.capacity = cap;
        this.items = T[cap];
        this.size = 0;
    }

    function add(item: T): Bool {
        if (this.size >= this.capacity) {
            return false;
        }
        this.items[this.size] = item;
        this.size = this.size + 1;
        return true;
    }

    function get(index: Int): T {
        return this.items[index];
    }

    function getSize(): Int {
        return this.size;
    }

    function getName(): String {
        return this.name;
    }

    function findFirst(target: T): Int {
        Int i = 0;
        while (i < this.size) {
            if (this.items[i] == target) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    function toArray(): T[] {
        T[] result = T[this.size];
        Int i = 0;
        while (i < this.size) {
            result[i] = this.items[i];
            i = i + 1;
        }
        return result;
    }
}

class Entity {
    Int id;
    String name;

    constructor(identifier: Int, entityName: String) {
        this.id = identifier;
        this.name = entityName;
    }

    function getId(): Int {
        return this.id;
    }

    function getName(): String {
        return this.name;
    }
}

print("Repository<Int> test:");
Repository<Int> intRepo = Repository<Int>("Numbers", 5);
print(intRepo.getName());

print(intRepo.add(10));
print(intRepo.add(20));
print(intRepo.add(30));

print("Size:");
print(intRepo.getSize());

print("Contents:");
Int i = 0;
while (i < intRepo.getSize()) {
    print(intRepo.get(i));
    i = i + 1;
}

print("Find 20:");
print(intRepo.findFirst(20));

print("Repository<String> test:");
Repository<String> strRepo = Repository<String>("Words", 4);
strRepo.add("alpha");
strRepo.add("beta");
strRepo.add("gamma");

String[] strArray = strRepo.toArray();
print("Exported array:");
i = 0;
while (i < 3) {
    print(strArray[i]);
    i = i + 1;
}

print("Repository<Entity> test:");
Repository<Entity> entityRepo = Repository<Entity>("Entities", 3);
entityRepo.add(Entity(1, "First"));
entityRepo.add(Entity(2, "Second"));
entityRepo.add(Entity(3, "Third"));

print("Entity repository:");
i = 0;
while (i < entityRepo.getSize()) {
    Entity e = entityRepo.get(i);
    print(e.getId());
    print(e.getName());
    i = i + 1;
}
