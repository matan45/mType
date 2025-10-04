// Test: Protected fields accessible within same class
class Vehicle {
    protected string model;
    protected int year;

    public constructor(string m, int y) {
        model = m;  // Accessing protected field in same class
        year = y;   // Accessing protected field in same class
    }

    public function getInfo(): string {
        // Accessing protected fields in same class
        return model + " (" + year + ")";
    }
}

Vehicle v = new Vehicle("Sedan", 2023);
print(v.getInfo());  // Expected: Sedan (2023)
