// Module: AlternateTypes
// Provides alternative class hierarchy with same names

namespace vehicles {
    public class Vehicle {
        protected string model;

        public constructor(string model) {
            this.model = model;
        }

        public string getModel() {
            return this.model;
        }
    }

    public class Car extends Vehicle {
        private string color;

        public constructor(string model, string color) : super(model) {
            this.color = color;
        }

        public string getColor() {
            return this.color;
        }
    }
}
