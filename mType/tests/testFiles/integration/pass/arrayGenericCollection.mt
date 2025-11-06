// Arrays + Generics Test 4: Generic collection backed by arrays
@Script

class ArrayList<T> {
    T[] data;
    Int size;
    Int capacity;

    constructor(Int initialCapacity) {
        this.capacity = initialCapacity;
        this.data = T[initialCapacity];
        this.size = 0;
    }

    public function add(T item): void {
        if (this.size >= this.capacity) {
            this.resize();
        }
        this.data[this.size] = item;
        this.size = this.size + 1;
    }

    public function resize(): void {
        Int newCapacity = this.capacity * 2;
        T[] newData = T[newCapacity];
        Int i = 0;
        while (i < this.size) {
            newData[i] = this.data[i];
            i = i + 1;
        }
        this.data = newData;
        this.capacity = newCapacity;
    }

    public function get(Int index): T {
        return this.data[index];
    }

    public function getSize(): Int {
        return this.size;
    }

    public function contains(T item): Bool {
        Int i = 0;
        while (i < this.size) {
            if (this.data[i] == item) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    public function remove(Int index): void {
        Int i = index;
        while (i < this.size - 1) {
            this.data[i] = this.data[i + 1];
            i = i + 1;
        }
        this.size = this.size - 1;
    }
}

ArrayList<Int> list = ArrayList<Int>(2);
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
Int i = 0;
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

ArrayList<String> strList = ArrayList<String>(3);
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
