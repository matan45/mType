class Car {
    string brand;
    string model;
    int year;
    float price;
    
    constructor(string b, string m, int y, float p) {
        brand = b;
        model = m;
        year = y;
        price = p;
    }
}

Car car = new Car("Toyota", "Camry", 2022, 25000.0);

// Access members
print(car.brand); // Toyota
print(car.model); // Camry
print(car.year); // 2022
print(car.price); // 25000.0

// Modify members
car.year = 2023;
car.price = 26000.0;

print(car.year); // 2023
print(car.price); // 26000.0