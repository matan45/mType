// Test detailed error messages for cascading type errors
// This should test error location reporting and detailed type mismatch messages

class Vehicle {
    string brand;
    int year;

    constructor(string b, int y) {
        brand = b;
        year = y;
    }

    string getInfo(): string {
        return brand;
    }
}

class Engine {
    int horsepower;
    float displacement;

    constructor(int hp, float d) {
        horsepower = hp;
        displacement = d;
    }

    int getPower(): int {
        return horsepower;
    }
}

class Person {
    string name;
    int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }

    string getName(): string {
        return name;
    }
}

function processVehicle(Vehicle v): string {
    return v.getInfo();
}

function calculatePower(Engine e): int {
    return e.getPower();
}

// Test 1: Function return type mismatch with detailed location
function getVehicleYear(): string {
    Vehicle v = new Vehicle("Toyota", 2020);
    return v.year;  // Error: returning int when string expected
}

// Test 2: Function parameter type mismatch with cascading errors
Vehicle car = new Vehicle("Honda", 2021);
Engine eng = new Engine(250, 2.5);
Person owner = new Person("John", 30);

// Error: passing Engine where Vehicle expected
string info1 = processVehicle(eng);

// Error: passing Person where Vehicle expected
string info2 = processVehicle(owner);

// Test 3: Assignment chain with incompatible types
Vehicle v1 = new Vehicle("Ford", 2019);
Engine e1 = new Engine(300, 3.0);
Person p1 = new Person("Alice", 25);

// Error: cannot assign Engine to Vehicle
v1 = e1;

// Test 4: Method call return type in wrong context
int powerValue = car.getInfo();  // Error: assigning string to int

// Test 5: Multiple parameter mismatches
function compareVehicles(Vehicle v1, Vehicle v2, int threshold): bool {
    return true;
}

// Error: wrong types for all parameters
bool result = compareVehicles(eng, owner, "invalid");

// Test 6: Nested function call type errors
function getVehicleCount(Vehicle v): int {
    return 1;
}

// Error: passing wrong type which produces wrong return type
string count = getVehicleCount(p1);  // Double error: wrong param and wrong assignment

// Test 7: Constructor parameter type mismatch with detailed message
Vehicle wrongVehicle = new Vehicle(2022, "BMW");  // Error: parameters in wrong order

// Test 8: Complex expression with multiple type errors
int complexCalc = (car.year + owner.age) * eng.getPower() + car.getInfo();  // Error: adding string to int
