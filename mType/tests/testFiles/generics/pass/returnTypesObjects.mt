// Test object/class return types
class Car {
    string model;
    public int year;

    constructor(string m, int y) {
        model = m;
        year = y;
    }

    public function getModel(): string {
        return model;
    }
}

function createCar(): Car {
    return new Car("Tesla", 2024);
}

function getCarModel(Car c): string {
    return c.getModel();
}

// Test functions
Car myCar = createCar();
print(myCar.getModel());
print(myCar.year);
