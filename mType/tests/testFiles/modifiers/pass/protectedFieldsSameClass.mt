// Test: Protected fields accessible within same class
class Vehicle {
    protected string model;
    protected int year;

    public Vehicle(string m, int y) {
        model = m;  // Accessing protected field in same class
        year = y;   // Accessing protected field in same class
    }

    public string getInfo() {
        // Accessing protected fields in same class
        return model + " (" + year.toString() + ")";
    }
}

Vehicle v = new Vehicle("Sedan", 2023);
print(v.getInfo());  // Expected: Sedan (2023)
