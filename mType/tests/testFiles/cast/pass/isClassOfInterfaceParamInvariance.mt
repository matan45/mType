// MYT-44: generic type arguments are invariant at the interface surface.
// `MyList<Dog>` is NOT a `MyList<Animal>`, even though Dog extends Animal.
// Pins invariance at the interface level, matching MYT-41's class-level
// behavior (Box<Dog> is not Box<Animal>).

interface MyList<T> {
    function get(int i): T;
}

class Animal {
    public string name;
    public constructor(string name) { this.name = name; }
}

class Dog extends Animal {
    public constructor(string name) : super(name) {}
}

class MyArrayList<E> implements MyList<E> {
    E slot;
    public constructor(E v) { this.slot = v; }
    public function get(int i): E { return this.slot; }
}

MyArrayList<Dog> dogs = new MyArrayList<Dog>(new Dog("rex"));

print(dogs isClassOf MyList<Dog>);    // true
print(dogs isClassOf MyList<Animal>); // false — invariant
print(dogs isClassOf MyList);         // true — raw fallback still works
