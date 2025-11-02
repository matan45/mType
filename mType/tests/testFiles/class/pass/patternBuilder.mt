// Test: Builder pattern implementation
// Expected: Pass - demonstrates builder design pattern

class Car {
    private string make;
    private string model;
    private int year;
    private string color;
    private bool hasSunroof;
    private bool hasNavigation;

    public constructor(string make, string model, int year, string color,
                      bool hasSunroof, bool hasNavigation) {
        this.make = make;
        this.model = model;
        this.year = year;
        this.color = color;
        this.hasSunroof = hasSunroof;
        this.hasNavigation = hasNavigation;
    }

    public string toString() {
        string result = this.year + " " + this.make + " " + this.model +
                       " (" + this.color + ")";
        if (this.hasSunroof) {
            result = result + " [Sunroof]";
        }
        if (this.hasNavigation) {
            result = result + " [Navigation]";
        }
        return result;
    }
}

class CarBuilder {
    private string make;
    private string model;
    private int year;
    private string color;
    private bool hasSunroof;
    private bool hasNavigation;

    public constructor() {
        // Default values
        this.make = "Generic";
        this.model = "Model";
        this.year = 2024;
        this.color = "White";
        this.hasSunroof = false;
        this.hasNavigation = false;
    }

    public CarBuilder setMake(string make) {
        this.make = make;
        print("Builder: Set make to " + make);
        return this;
    }

    public CarBuilder setModel(string model) {
        this.model = model;
        print("Builder: Set model to " + model);
        return this;
    }

    public CarBuilder setYear(int year) {
        this.year = year;
        print("Builder: Set year to " + year);
        return this;
    }

    public CarBuilder setColor(string color) {
        this.color = color;
        print("Builder: Set color to " + color);
        return this;
    }

    public CarBuilder withSunroof() {
        this.hasSunroof = true;
        print("Builder: Added sunroof");
        return this;
    }

    public CarBuilder withNavigation() {
        this.hasNavigation = true;
        print("Builder: Added navigation");
        return this;
    }

    public Car build() {
        print("Builder: Building car");
        return new Car(this.make, this.model, this.year, this.color,
                      this.hasSunroof, this.hasNavigation);
    }
}

// Test builder pattern
print("Test 1: Build with all options");
Car car1 = new CarBuilder()
    .setMake("Tesla")
    .setModel("Model S")
    .setYear(2024)
    .setColor("Red")
    .withSunroof()
    .withNavigation()
    .build();
print("Result: " + car1.toString());

print("\nTest 2: Build with minimal options");
Car car2 = new CarBuilder()
    .setMake("Toyota")
    .setModel("Camry")
    .build();
print("Result: " + car2.toString());

print("\nTest 3: Build step by step");
CarBuilder builder = new CarBuilder();
builder.setMake("Honda");
builder.setModel("Civic");
builder.setColor("Blue");
builder.withNavigation();
Car car3 = builder.build();
print("Result: " + car3.toString());
