@Script
// Test runtime type check with static cast
class Vehicle {
    fn start(): String {
        return "Vehicle starting";
    }
}

class Car : Vehicle {
    fn start(): String {
        return "Car starting";
    }

    fn openTrunk(): String {
        return "Trunk opened";
    }
}

class Motorcycle : Vehicle {
    fn start(): String {
        return "Motorcycle starting";
    }

    fn popWheelie(): String {
        return "Wheelie!";
    }
}

fn identifyVehicle(v: Vehicle): String {
    let car: Car = v as Car;
    if (car != null) {
        return "It's a car";
    }

    let moto: Motorcycle = v as Motorcycle;
    if (moto != null) {
        return "It's a motorcycle";
    }

    return "Unknown vehicle";
}

fn main() {
    let car: Car = new Car();
    let moto: Motorcycle = new Motorcycle();

    print(identifyVehicle(car as Vehicle));
    print(identifyVehicle(moto as Vehicle));

    let v: Vehicle = car as Vehicle;
    let c: Car = v as Car;
    if (c != null) {
        print(c.openTrunk());
    }
}
