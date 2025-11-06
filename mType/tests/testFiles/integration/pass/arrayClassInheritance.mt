// Arrays + Class Test 2: Arrays in inheritance hierarchies
@Script

class Vehicle {
    Int wheels;

    constructor(w: Int) {
        this.wheels = w;
    }

    function getWheels(): Int {
        return this.wheels;
    }

    function start(): String {
        return "Vehicle started";
    }
}

class Car extends Vehicle {
    Int doors;

    constructor(d: Int) {
        super(4);
        this.doors = d;
    }

    function getDoors(): Int {
        return this.doors;
    }

    function start(): String {
        return "Car engine started";
    }
}

class Motorcycle extends Vehicle {
    Bool hasKickstand;

    constructor() {
        super(2);
        this.hasKickstand = true;
    }

    function start(): String {
        return "Motorcycle ignited";
    }
}

class Garage {
    Vehicle[] vehicles;
    Int capacity;
    Int count;

    constructor(cap: Int) {
        this.capacity = cap;
        this.vehicles = Vehicle[cap];
        this.count = 0;
    }

    function addVehicle(v: Vehicle): void {
        if (this.count < this.capacity) {
            this.vehicles[this.count] = v;
            this.count = this.count + 1;
        }
    }

    function startAll(): void {
        Int i = 0;
        while (i < this.count) {
            print(this.vehicles[i].start());
            i = i + 1;
        }
    }

    function countWheels(): Int {
        Int total = 0;
        Int i = 0;
        while (i < this.count) {
            total = total + this.vehicles[i].getWheels();
            i = i + 1;
        }
        return total;
    }
}

print("Creating garage:");
Garage garage = Garage(5);

print("Adding vehicles:");
garage.addVehicle(Car(2));
garage.addVehicle(Car(4));
garage.addVehicle(Motorcycle());
garage.addVehicle(Vehicle(6));
garage.addVehicle(Car(2));

print("Starting all vehicles:");
garage.startAll();

print("Total wheels:");
print(garage.countWheels());

print("Direct array test:");
Car[] cars = Car[3];
cars[0] = Car(2);
cars[1] = Car(4);
cars[2] = Car(2);

Int i = 0;
while (i < 3) {
    print(cars[i].getDoors());
    print(cars[i].getWheels());
    i = i + 1;
}
