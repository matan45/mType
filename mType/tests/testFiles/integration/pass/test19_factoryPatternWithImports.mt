// Integration Test 19: Factory Pattern with Imports
// Tests: Imports + Static methods + Abstract classes + Polymorphism

import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Abstract product
abstract class Vehicle {
    protected string type;
    protected int speed;

    constructor(string t, int s) {
        this.type = t;
        this.speed = s;
    }

    abstract function start(): void;
    abstract function stop(): void;

    public function getInfo(): string {
        return "Vehicle: " + this.type + ", Speed: " + this.speed;
    }

    public function getType(): string {
        return this.type;
    }
}

// Concrete products
class Car extends Vehicle {
    private int doors;

    constructor(int s, int d) : super("Car", s) {
        this.doors = d;
    }

    public function start(): void {
        print("Car engine starting...");
    }

    public function stop(): void {
        print("Car engine stopping...");
    }

    public function getDoors(): int {
        return this.doors;
    }
}

class Motorcycle extends Vehicle {
    private bool hasSidecar;

    constructor(int s, bool sc) : super("Motorcycle", s) {
        this.hasSidecar = sc;
    }

    public function start(): void {
        print("Motorcycle ignition on...");
    }

    public function stop(): void {
        print("Motorcycle ignition off...");
    }
}

class Truck extends Vehicle {
    private int cargoCapacity;

    constructor(int s, int cargo) : super("Truck", s) {
        this.cargoCapacity = cargo;
    }

    public function start(): void {
        print("Truck diesel engine starting...");
    }

    public function stop(): void {
        print("Truck diesel engine stopping...");
    }

    public function getCargoCapacity(): int {
        return this.cargoCapacity;
    }
}

// Factory class with static methods
class VehicleFactory {
    private static int vehicleCount = 0;

    // Factory method
    public static function createVehicle(string type): Vehicle {
        VehicleFactory::vehicleCount = VehicleFactory::vehicleCount + 1;
        print("Factory: Creating vehicle #" + VehicleFactory::vehicleCount);

        if (type == "car") {
            return new Car(120, 4);
        } else if (type == "motorcycle") {
            return new Motorcycle(180, false);
        } else if (type == "truck") {
            return new Truck(90, 5000);
        } else {
            print("Unknown vehicle type, creating default car");
            return new Car(100, 4);
        }
    }

    public static function getVehicleCount(): int {
        return VehicleFactory::vehicleCount;
    }
}

// Abstract factory pattern
interface DocumentFactory {
    function createDocument(): Document;
    function createTemplate(): Template;
}

abstract class Document {
    protected string title;

    constructor(string t) {
        this.title = t;
    }

    abstract function render(): void;

    public function getTitle(): string {
        return this.title;
    }
}

abstract class Template {
    protected string name;

    constructor(string n) {
        this.name = n;
    }

    abstract function apply(): void;
}

// Concrete document types
class PDFDocument extends Document {
    constructor(string t) : super(t) {}

    public function render(): void {
        print("Rendering PDF: " + this.title);
    }
}

class HTMLDocument extends Document {
    constructor(string t) : super(t) {}

    public function render(): void {
        print("Rendering HTML: " + this.title);
    }
}

class PDFTemplate extends Template {
    constructor(string n) : super(n) {}

    public function apply(): void {
        print("Applying PDF template: " + this.name);
    }
}

class HTMLTemplate extends Template {
    constructor(string n) : super(n) {}

    public function apply(): void {
        print("Applying HTML template: " + this.name);
    }
}

// Concrete factories
class PDFFactory implements DocumentFactory {
    public function createDocument(): Document {
        return new PDFDocument("PDF Report");
    }

    public function createTemplate(): Template {
        return new PDFTemplate("PDF Standard");
    }
}

class HTMLFactory implements DocumentFactory {
    public function createDocument(): Document {
        return new HTMLDocument("Web Page");
    }

    public function createTemplate(): Template {
        return new HTMLTemplate("HTML5 Boilerplate");
    }
}

// Client code
function testVehicles(Vehicle v): void {
    print("Testing: " + v.getInfo());
    v.start();
    v.stop();
}

// Main test execution
print("=== Test 19: Factory Pattern with Imports ===");

// Test 1: Simple factory
print("--- Test 1: Vehicle factory ---");
Vehicle car = VehicleFactory::createVehicle("car");
testVehicles(car);

Vehicle motorcycle = VehicleFactory::createVehicle("motorcycle");
testVehicles(motorcycle);

Vehicle truck = VehicleFactory::createVehicle("truck");
testVehicles(truck);

print("Total vehicles created: " + VehicleFactory::getVehicleCount());

// Test 2: Type checking
print("--- Test 2: Type checking ---");
if (car isClassOf Car) {
    Car c = (Car)car;
    print("Car has " + c.getDoors() + " doors");
}

if (truck isClassOf Truck) {
    Truck t = (Truck)truck;
    print("Truck capacity: " + t.getCargoCapacity() + " kg");
}

// Test 3: Abstract factory
print("--- Test 3: Abstract factory (PDF) ---");
DocumentFactory pdfFactory = new PDFFactory();
Document pdfDoc = pdfFactory.createDocument();
Template pdfTemplate = pdfFactory.createTemplate();

pdfTemplate.apply();
pdfDoc.render();

print("--- Test 3: Abstract factory (HTML) ---");
DocumentFactory htmlFactory = new HTMLFactory();
Document htmlDoc = htmlFactory.createDocument();
Template htmlTemplate = htmlFactory.createTemplate();

htmlTemplate.apply();
htmlDoc.render();

// Test 4: Factory method with unknown type
print("--- Test 4: Unknown vehicle type ---");
Vehicle unknown = VehicleFactory::createVehicle("airplane");
print(unknown.getInfo());

// Test 5: Multiple vehicles of same type
print("--- Test 5: Multiple vehicles ---");
Vehicle car1 = VehicleFactory::createVehicle("car");
Vehicle car2 = VehicleFactory::createVehicle("car");
Vehicle car3 = VehicleFactory::createVehicle("car");

print("Total vehicles created: " + VehicleFactory::getVehicleCount());

// Test 6: Polymorphic array
print("--- Test 6: Polymorphic array ---");
Vehicle[] fleet = new Vehicle[3];
fleet[0] = VehicleFactory::createVehicle("car");
fleet[1] = VehicleFactory::createVehicle("motorcycle");
fleet[2] = VehicleFactory::createVehicle("truck");

print("Fleet test:");
for (int i = 0; i < fleet.length; i = i + 1) {
    print("Vehicle " + i + ": " + fleet[i].getType());
}

print("Final vehicle count: " + VehicleFactory::getVehicleCount());

print("=== Test 19 Complete ===");
