// Arrays + Class Test 2: Arrays in inheritance hierarchies
@Script

class Vehicle {
    field wheels: Int;

    constructor(w: Int) {
        this.wheels = w;
    }

    fun getWheels(): Int {
        return this.wheels;
    }

    fun start(): String {
        return "Vehicle started";
    }
}

class Car extends Vehicle {
    field doors: Int;

    constructor(d: Int) {
        super(4);
        this.doors = d;
    }

    fun getDoors(): Int {
        return this.doors;
    }

    fun start(): String {
        return "Car engine started";
    }
}

class Motorcycle extends Vehicle {
    field hasKickstand: Bool;

    constructor() {
        super(2);
        this.hasKickstand = true;
    }

    fun start(): String {
        return "Motorcycle ignited";
    }
}

class Garage {
    field vehicles: Vehicle[];
    field capacity: Int;
    field count: Int;

    constructor(cap: Int) {
        this.capacity = cap;
        this.vehicles = Vehicle[cap];
        this.count = 0;
    }

    fun addVehicle(v: Vehicle): Void {
        if (this.count < this.capacity) {
            this.vehicles[this.count] = v;
            this.count = this.count + 1;
        }
    }

    fun startAll(): Void {
        let i: Int = 0;
        while (i < this.count) {
            print(this.vehicles[i].start());
            i = i + 1;
        }
    }

    fun countWheels(): Int {
        let total: Int = 0;
        let i: Int = 0;
        while (i < this.count) {
            total = total + this.vehicles[i].getWheels();
            i = i + 1;
        }
        return total;
    }
}

print("Creating garage:");
let garage: Garage = Garage(5);

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
let cars: Car[] = Car[3];
cars[0] = Car(2);
cars[1] = Car(4);
cars[2] = Car(2);

let i: Int = 0;
while (i < 3) {
    print(cars[i].getDoors());
    print(cars[i].getWheels());
    i = i + 1;
}
