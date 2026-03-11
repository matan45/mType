// Null matches two unrelated class types equally - should be ambiguous
class Cat {
    constructor() {}
}

class Car {
    constructor() {}
}

function identify(Cat c): string {
    return "Cat";
}

function identify(Car c): string {
    return "Car";
}

@Script
function main(): void {
    // ERROR: null matches both Cat and Car as EXACT_MATCH with distance 0
    print(identify(null));
}
