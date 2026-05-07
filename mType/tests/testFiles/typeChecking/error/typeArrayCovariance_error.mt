// MYT-137: variable-to-variable array assignment is invariant. A `Car[]`
// cannot be aliased through a `Vehicle[]` declaration because once the
// alias exists a `vehicles[i] = new Motorcycle(...)` would corrupt the
// underlying typed Car-array storage. The strict check at
// TypeValidator::validateObjectTypeAssignment rejects this at compile
// time; the historical covariant escape was removed.
//
// (Pre-MYT-137 this file relied on a parse error in `string getBrand():
// string` to pass as ERROR_EXPECTED — a false positive that did not
// actually exercise the covariance path. Body now uses correct mType
// syntax so the test pins the real check.)

class Vehicle {
    string brand;

    constructor(string b) {
        brand = b;
    }

    public function getBrand(): string {
        return brand;
    }
}

class Car extends Vehicle {
    int doors;

    constructor(string b, int d) : super(b) {
        doors = d;
    }

    public function getDoors(): int {
        return doors;
    }
}

class Motorcycle extends Vehicle {
    bool hasSidecar;

    constructor(string b, bool s) : super(b) {
        hasSidecar = s;
    }
}

function main(): void {
    Car[] cars = new Car[2];
    cars[0] = new Car("Toyota", 4);
    cars[1] = new Car("Honda", 2);

    // ERROR: array invariance — Car[] cannot be assigned to Vehicle[]
    // because the alias would let a Motorcycle store corrupt cars[].
    Vehicle[] vehicles = cars;

    print("This should not execute");
}

main();
