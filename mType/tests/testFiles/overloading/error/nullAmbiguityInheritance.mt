// null passed to overloads where one type is a subclass of the other.
// null is assignable to both Animal and Dog at EXACT_MATCH distance 0,
// so overload resolution must report ambiguity rather than silently picking
// the most-derived type.
class Animal {
    constructor() {}
}

class Dog extends Animal {
    constructor() {}
}

function describe(Animal a): string {
    return "animal";
}

function describe(Dog d): string {
    return "dog";
}

function main(): void {
    // ERROR: ambiguous call to 'describe' - null matches both Animal and Dog
    print(describe(null));
}
