import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Safe casting with variance
class Vehicle {
    String type;

    public function Vehicle(String t) {
        type = t;
    }

    public function getType(): String {
        return type;
    }
}

class Car extends Vehicle {
    public function Car() {
        super(new String("Car"));
    }
}

class Garage<T extends Vehicle> {
    T vehicle;

    public function park(T v): void {
        vehicle = v;
    }

    public function retrieve(): T {
        return vehicle;
    }

    public function inspect(): String {
        return vehicle.getType();
    }
}

function inspectGarage(Garage<Vehicle> garage): void {
    print("Vehicle type: " + garage.inspect());
}

function main(): void {
    Garage<Car> carGarage = new Garage<Car>();
    carGarage.park(new Car());

    // Safe upcast for inspection
    inspectGarage(carGarage);
}

main();
