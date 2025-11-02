// Test: Casting with instanceof/isClassOf combination
class Vehicle {
    public int wheels;

    public Vehicle(int w) {
        this.wheels = w;
    }
}

class Car extends Vehicle {
    public int doors;

    public Car(int w, int d) {
        super(w);
        this.doors = d;
    }

    public function getInfo(): String {
        return "Car with " + this.doors + " doors";
    }
}

class Motorcycle extends Vehicle {
    public bool hasSidecar;

    public Motorcycle(int w, bool s) {
        super(w);
        this.hasSidecar = s;
    }

    public function getInfo(): String {
        return hasSidecar ? "Motorcycle with sidecar" : "Motorcycle";
    }
}

// Safe casting pattern with isClassOf
Vehicle v1 = new Car(4, 4);
if (v1 isClassOf Car) {
    print(((Car)v1).getInfo());
    print("Doors: " + ((Car)v1).doors);
}

// Multiple type checks with casts
Vehicle v2 = new Motorcycle(2, true);
if (v2 isClassOf Motorcycle) {
    print(((Motorcycle)v2).getInfo());
    if (((Motorcycle)v2).hasSidecar) {
        print("Has sidecar");
    }
} else if (v2 isClassOf Car) {
    print(((Car)v2).getInfo());
}

// Guard pattern - type check before complex cast expression
Vehicle v3 = new Car(4, 2);
if (v3 isClassOf Car && ((Car)v3).doors > 1) {
    print("Multi-door car: " + ((Car)v3).getInfo());
}

// Nested type checks with casts
Vehicle vehicles[] = new Vehicle[3];
vehicles[0] = new Car(4, 4);
vehicles[1] = new Motorcycle(2, false);
vehicles[2] = new Car(4, 2);

for (int i = 0; i < 3; i = i + 1) {
    if (vehicles[i] isClassOf Car) {
        if (((Car)vehicles[i]).doors == 4) {
            print("4-door car");
        } else {
            print("2-door car");
        }
    } else if (vehicles[i] isClassOf Motorcycle) {
        if (((Motorcycle)vehicles[i]).hasSidecar) {
            print("Motorcycle with sidecar");
        } else {
            print("Standard motorcycle");
        }
    }
}

// Expected output:
// Car with 4 doors
// Doors: 4
// Motorcycle with sidecar
// Has sidecar
// Multi-door car: Car with 2 doors
// 4-door car
// Standard motorcycle
// 2-door car
