// Module: ExtendedTypes
// Extends types from BaseTypes module

import { Vehicle, Car } from "./BaseTypes.mt";

namespace extended {
    public class ElectricCar extends types.Car {
        private int batteryCapacity;

        public constructor(string brand, int doors, int batteryCapacity) : super(brand, doors) {
            this.batteryCapacity = batteryCapacity;
        }

        public void start() {
            print("ElectricCar silent start: " + this.brand + " (" + this.batteryCapacity + " kWh)");
        }

        public int getBatteryCapacity() {
            return this.batteryCapacity;
        }
    }

    public class Truck extends types.Vehicle {
        private int payload;

        public constructor(string brand, int payload) : super(brand) {
            this.payload = payload;
        }

        public void start() {
            print("Truck starting: " + this.brand + " (payload: " + this.payload + " tons)");
        }

        public int getPayload() {
            return this.payload;
        }
    }
}
