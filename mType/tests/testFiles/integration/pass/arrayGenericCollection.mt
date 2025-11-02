// Arrays + Generics Test 4: Generic collection backed by arrays
@Script

class ArrayList<T> {
    field data: T[];
    field size: Int;
    field capacity: Int;

    constructor(initialCapacity: Int) {
        this.capacity = initialCapacity;
        this.data = T[initialCapacity];
        this.size = 0;
    }

    fun add(item: T): Void {
        if (this.size >= this.capacity) {
            this.resize();
        }
        this.data[this.size] = item;
        this.size = this.size + 1;
    }

    fun resize(): Void {
        let newCapacity: Int = this.capacity * 2;
        let newData: T[] = T[newCapacity];
        let i: Int = 0;
        while (i < this.size) {
            newData[i] = this.data[i];
            i = i + 1;
        }
        this.data = newData;
        this.capacity = newCapacity;
    }

    fun get(index: Int): T {
        return this.data[index];
    }

    fun getSize(): Int {
        return this.size;
    }

    fun contains(item: T): Bool {
        let i: Int = 0;
        while (i < this.size) {
            if (this.data[i] == item) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    fun remove(index: Int): Void {
        let i: Int = index;
        while (i < this.size - 1) {
            this.data[i] = this.data[i + 1];
            i = i + 1;
        }
        this.size = this.size - 1;
    }
}

let list: ArrayList<Int> = ArrayList<Int>(2);
print("Initial capacity: 2");

list.add(10);
list.add(20);
print("Added 10, 20");

list.add(30);
print("Added 30 (triggers resize)");

list.add(40);
list.add(50);
print("Added 40, 50");

print("List contents:");
let i: Int = 0;
while (i < list.getSize()) {
    print(list.get(i));
    i = i + 1;
}

print("Contains 30:");
print(list.contains(30));

print("Contains 100:");
print(list.contains(100));

list.remove(2);
print("After removing index 2:");
i = 0;
while (i < list.getSize()) {
    print(list.get(i));
    i = i + 1;
}

let strList: ArrayList<String> = ArrayList<String>(3);
strList.add("alpha");
strList.add("beta");
strList.add("gamma");
strList.add("delta");

print("String list:");
i = 0;
while (i < strList.getSize()) {
    print(strList.get(i));
    i = i + 1;
}
