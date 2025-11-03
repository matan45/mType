
// Test runtime type check with static cast
class Vehicle {
    public function start(): string {
        return "Vehicle starting";
    }
}

class Car extends Vehicle {
    public function start(): string {
        return "Car starting";
    }

    public function openTrunk(): string {
        return "Trunk opened";
    }
}

class Motorcycle extends Vehicle {
    public function start(): string {
        return "Motorcycle starting";
    }

    public function popWheelie(): string {
        return "Wheelie!";
    }
}

function identifyVehicle(Vehicle v): string {
    // Use isClassOf to check type before casting
    if (v isClassOf Car) {
        Car car = (Car)v;
        return "It's a car";
    }

    if (v isClassOf Motorcycle) {
        Motorcycle moto = (Motorcycle)v;
        return "It's a motorcycle";
    }

    return "Unknown vehicle";
}

function main(): void {
    Car car = new Car();
    Motorcycle moto = new Motorcycle();

    print(identifyVehicle((Vehicle)car));
    print(identifyVehicle((Vehicle)moto));

    Vehicle v = (Vehicle)car;
    Car c = (Car)v;
    if (c != null) {
        print(c.openTrunk());
    }
}

main();
