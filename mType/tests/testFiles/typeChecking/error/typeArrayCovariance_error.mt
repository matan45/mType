// ERROR: Test array covariance violation - derived type array cannot be assigned to base type array

class Vehicle {
    string brand;

    constructor(string b) {
        brand = b;
    }

    string getBrand(): string {
        return brand;
    }
}

class Car extends Vehicle {
    int doors;

    constructor(string b, int d) {
        super(b);
        doors = d;
    }

    int getDoors(): int {
        return doors;
    }
}

class Motorcycle extends Vehicle {
    bool hasSidecar;

    constructor(string b, bool s) {
        super(b);
        hasSidecar = s;
    }
}

function main(): void {
    // Create Car array
    Car[] cars = new Car[2];
    cars[0] = new Car("Toyota", 4);
    cars[1] = new Car("Honda", 2);

    // ERROR: Array covariance violation - cannot assign Car[] to Vehicle[]
    // This would be unsafe because we could then add a Motorcycle to the array
    Vehicle[] vehicles = cars;

    // This would corrupt the cars array if the above were allowed:
    // vehicles[0] = new Motorcycle("Harley", false);

    print("This should not execute");
}

main();
