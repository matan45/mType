class Phone {
    string model;
    constructor(string m) {
        model = m;
    }
}

class Computer {
    string brand;
    constructor(string b) {
        brand = b;
    }
}

Phone phone = new Computer("Dell");  // Should fail