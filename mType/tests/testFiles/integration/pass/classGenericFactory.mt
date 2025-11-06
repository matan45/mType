// Class + Generics Test 5: Generic factory pattern
@Script

class Item<T> {
    Int id;
    T data;

    constructor(Int identifier, T value) {
        this.id = identifier;
        this.data = value;
    }

    public function getId(): Int {
        return this.id;
    }

    public function getData(): T {
        return this.data;
    }

    public function display(): void {
        print(this.id);
        print(this.data);
    }
}

class Factory<T> {
    Int counter;

    constructor() {
        this.counter = 0;
    }

    public function create(T data): Item<T> {
        Item<T> item = Item<T>(this.counter, data);
        this.counter = this.counter + 1;
        return item;
    }

    public function createBatch(T data, Int count): Item<T>[] {
        Item<T>[] items = Item<T>[count];
        Int i = 0;
        while (i < count) {
            items[i] = this.create(data);
            i = i + 1;
        }
        return items;
    }

    public function getCounter(): Int {
        return this.counter;
    }
}

print("Creating Int factory:");
Factory<Int> intFactory = Factory<Int>();

Item<Int> item1 = intFactory.create(100);
print("Item 1:");
item1.display();

Item<Int> item2 = intFactory.create(200);
print("Item 2:");
item2.display();

print("Factory counter:");
print(intFactory.getCounter());

print("Creating batch:");
Item<Int>[] batch = intFactory.createBatch(999, 3);
Int i = 0;
while (i < 3) {
    print("Batch item:");
    batch[i].display();
    i = i + 1;
}

print("Factory counter after batch:");
print(intFactory.getCounter());

print("Creating String factory:");
Factory<String> strFactory = Factory<String>();
Item<String> strItem1 = strFactory.create("alpha");
Item<String> strItem2 = strFactory.create("beta");

print("String items:");
strItem1.display();
strItem2.display();

print("Creating Float factory:");
Factory<Float> floatFactory = Factory<Float>();
Item<Float>[] floatItems = floatFactory.createBatch(3.14, 2);
print("Float batch:");
i = 0;
while (i < 2) {
    floatItems[i].display();
    i = i + 1;
}
