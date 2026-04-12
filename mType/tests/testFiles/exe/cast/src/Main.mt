// Test: Cast in standalone exe

class Vehicle {
    protected string make;

    public constructor(string m) {
        this.make = m;
    }

    public function getMake(): string {
        return this.make;
    }
}

class Car extends Vehicle {
    private int doors;

    public constructor(string m, int d) : super(m) {
        this.doors = d;
    }

    public function getDoors(): int {
        return this.doors;
    }
}

class Truck extends Vehicle {
    private float payload;

    public constructor(string m, float p) : super(m) {
        this.payload = p;
    }

    public function getPayload(): float {
        return this.payload;
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Upcast (implicit)
        Car car = new Car("Toyota", 4);
        Vehicle v = car;
        print("Upcast make: " + v.getMake());

        // Downcast with isClassOf check
        if (v isClassOf Car) {
            Car c = (Car)v;
            print("Downcast doors: " + c.getDoors());
        }

        // Wrong cast prevented by isClassOf
        if (v isClassOf Truck) {
            print("Should not print");
        } else {
            print("Correctly identified: not a Truck");
        }

        // Cast in array processing
        Vehicle[] vehicles = new Vehicle[3];
        vehicles[0] = new Car("Honda", 4);
        vehicles[1] = new Truck("Ford", 5.0);
        vehicles[2] = new Car("BMW", 2);

        int totalDoors = 0;
        float totalPayload = 0.0;
        for (int i = 0; i < 3; i = i + 1) {
            if (vehicles[i] isClassOf Car) {
                Car c = (Car)vehicles[i];
                totalDoors = totalDoors + c.getDoors();
            } else if (vehicles[i] isClassOf Truck) {
                Truck t = (Truck)vehicles[i];
                totalPayload = totalPayload + t.getPayload();
            }
        }
        print("Total doors: " + totalDoors);
        print("Total payload: " + totalPayload);

        print("Cast test passed");
    }
}
