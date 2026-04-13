import * from "../../lib/reflect/Class.mt";

class Vehicle {
    public string brand;
    public int year;

    public constructor(string brand, int year) {
        this.brand = brand;
        this.year = year;
    }

    public function describe(): string {
        return this.brand + " " + this.year;
    }
}

Vehicle v = new Vehicle("Toyota", 2024);
print(v.describe());
Class vehicleClass = Class::forName("Vehicle");
print(vehicleClass.getName());
