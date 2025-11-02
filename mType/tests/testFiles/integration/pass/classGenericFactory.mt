// Class + Generics Test 5: Generic factory pattern
@Script

class Item<T> {
    field id: Int;
    field data: T;

    constructor(identifier: Int, value: T) {
        this.id = identifier;
        this.data = value;
    }

    fun getId(): Int {
        return this.id;
    }

    fun getData(): T {
        return this.data;
    }

    fun display(): Void {
        print(this.id);
        print(this.data);
    }
}

class Factory<T> {
    field counter: Int;

    constructor() {
        this.counter = 0;
    }

    fun create(data: T): Item<T> {
        let item: Item<T> = Item<T>(this.counter, data);
        this.counter = this.counter + 1;
        return item;
    }

    fun createBatch(data: T, count: Int): Item<T>[] {
        let items: Item<T>[] = Item<T>[count];
        let i: Int = 0;
        while (i < count) {
            items[i] = this.create(data);
            i = i + 1;
        }
        return items;
    }

    fun getCounter(): Int {
        return this.counter;
    }
}

print("Creating Int factory:");
let intFactory: Factory<Int> = Factory<Int>();

let item1: Item<Int> = intFactory.create(100);
print("Item 1:");
item1.display();

let item2: Item<Int> = intFactory.create(200);
print("Item 2:");
item2.display();

print("Factory counter:");
print(intFactory.getCounter());

print("Creating batch:");
let batch: Item<Int>[] = intFactory.createBatch(999, 3);
let i: Int = 0;
while (i < 3) {
    print("Batch item:");
    batch[i].display();
    i = i + 1;
}

print("Factory counter after batch:");
print(intFactory.getCounter());

print("Creating String factory:");
let strFactory: Factory<String> = Factory<String>();
let strItem1: Item<String> = strFactory.create("alpha");
let strItem2: Item<String> = strFactory.create("beta");

print("String items:");
strItem1.display();
strItem2.display();

print("Creating Float factory:");
let floatFactory: Factory<Float> = Factory<Float>();
let floatItems: Item<Float>[] = floatFactory.createBatch(3.14, 2);
print("Float batch:");
i = 0;
while (i < 2) {
    floatItems[i].display();
    i = i + 1;
}
