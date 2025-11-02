// Module: BaseTypes
// Provides base classes for casting tests

namespace types {
    public class Vehicle {
        protected string brand;

        public constructor(string brand) {
            this.brand = brand;
        }

        public string getBrand() {
            return this.brand;
        }

        public void start() {
            print("Vehicle starting: " + this.brand);
        }
    }

    public class Car extends Vehicle {
        private int doors;

        public constructor(string brand, int doors) : super(brand) {
            this.doors = doors;
        }

        public void start() {
            print("Car starting: " + this.brand + " with " + this.doors + " doors");
        }

        public int getDoors() {
            return this.doors;
        }
    }

    public class SportsCar extends Car {
        private int topSpeed;

        public constructor(string brand, int topSpeed) : super(brand, 2) {
            this.topSpeed = topSpeed;
        }

        public void start() {
            print("SportsCar roaring: " + this.brand + " (top speed: " + this.topSpeed + " mph)");
        }

        public int getTopSpeed() {
            return this.topSpeed;
        }
    }
}
