// Tier B: assigning Cat[] to Array<Dog>. The skip at TypeValidator.cpp:203-207
// allows compatible "T[] -> Array<T>" cross-format assignment; verify it does
// not also let *incompatible* element types through.
// Targets: TypeValidator.cpp:203-207.

class Animal {
    public constructor() {}
}

class Dog extends Animal {
    public constructor() {}
}

class Cat extends Animal {
    public constructor() {}
}

Cat[] cats = new Cat[1];
cats[0] = new Cat();

// Cat[] -> Array<Dog> must error: skip path is for *matching* element types only.
Array<Dog> dogs = cats;

print("should not reach");
