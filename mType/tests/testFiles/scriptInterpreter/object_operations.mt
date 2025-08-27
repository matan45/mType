// Basic class with instance methods
class Vehicle {
    string brand;
    string model;
    int year;
    
    constructor(string b, string m, int y) {
        brand = b;
        model = m;
        year = y;
    }
    
    function getInfo(): string {
        return brand + " " + model + " (" + toString(year) + ")";
    }
    
    function getAge(): int {
        return 2024 - this.year;
    }
    
    function updateYear(int newYear): void {
        this.year = newYear;
    }
    
    function setBrand(string newBrand): void {
        this.brand = newBrand;
    }
}

// Class with more complex methods
class Calculator {
    string name;
    
    constructor(string n) {
        name = n;
    }
    
    function add(int a, int b): int {
        return a + b;
    }
    
    function multiply(float a, float b): float {
        return a * b;
    }
    
    function getName(): string {
        return name;
    }
}